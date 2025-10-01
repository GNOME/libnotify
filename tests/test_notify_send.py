# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option) any
# later version.  See http://www.gnu.org/copyleft/lgpl.html for the full text
# of the license.

__author__ = "Martin Pitt"
__copyright__ = """
(c) 2012 Canonical Ltd.
(c) 2017 - 2022 Martin Pitt <martin@piware.de>
(c) 2025 Marco Trevisan <mail@3v1n0.net>
"""

import dbus
import dbusmock
import fcntl
import os
import shutil
import subprocess
import signal
import sys
import tempfile
import unittest

DBUS_IFACE = "org.freedesktop.Notifications"

DEFAULT_CAPS = [
    "body",
    "body-markup",
    "icon-static",
    "image/svg+xml",
    "private-synchronous",
    "append",
    "private-icon-only",
    "truncation",
]

notify_send = os.environ.get("NOTIFY_SEND", None)

if not notify_send:
    notify_send = shutil.which("notify-send")

subprocess.check_output([notify_send, "--version"], text=True)

class TemporaryFD:
    """ Create a temporary FD, do not use memfd as it may be less portable """
    def __init__(self, prefix="temporary-fd"):
        self.prefix = prefix
        self.fd = -1
        self.temporary = None

    def __enter__(self):
        self.temporary = tempfile.NamedTemporaryFile(prefix=self.prefix)
        self.fd = os.open(self.temporary.name, os.O_RDWR|os.O_CREAT)
        return [self.fd, self.temporary]

    def __exit__(self, exc_type, exc_value, traceback):
        os.close(self.fd)
        self.temporary.close()



class TestBaseNotifySend(dbusmock.DBusTestCase):
    """Test mocking notification-daemon"""

    @classmethod
    def setUpClass(cls):
        cls.start_session_bus()
        cls.dbus_con = cls.get_dbus(False)
        cls.caps = DEFAULT_CAPS.copy()
        cls.template = None
        cls.env = os.environ.copy()

    @classmethod
    def get_local_asset(self, *args):
        path = os.path.join(os.path.dirname(__file__), *args)
        self.assertTrue(os.path.exists(path), f"{path} does not exist")
        return path

    def setUp(self):
        (self.p_mock, self.obj_daemon) = self.spawn_server_template(
            self.template, {"capabilities": " ".join(self.caps)},
            stdout=subprocess.PIPE,
        )
        # set log to nonblocking
        flags = fcntl.fcntl(self.p_mock.stdout, fcntl.F_GETFL)
        fcntl.fcntl(self.p_mock.stdout, fcntl.F_SETFL, flags | os.O_NONBLOCK)

    def tearDown(self):
        self.p_mock.stdout.close()
        self.p_mock.terminate()
        self.p_mock.wait()

    def assertDaemonCall(self, method):
        calls = self.obj_daemon.GetMethodCalls(method)
        self.assertEqual(len(calls), 1)
        [ret, args] = calls[0]
        print(method, "Called with:", args, "=>", ret)
        return args

    def notify_send_proc(self, args=[], stdout=None, stderr=None, pass_fds=[]):
        print("Launching", [notify_send] + args, "With FDs", pass_fds)
        ns_proc = subprocess.Popen([notify_send] + args, env=self.env,
                                   stdout=stdout, stderr=stderr,
                                   pass_fds=pass_fds)
        return ns_proc

    def notify_send(self, args=[]):
        ns_proc = self.notify_send_proc(args)
        ns_proc.wait()
        self.assertEqual(ns_proc.returncode, 0)
        return ns_proc

    def notify_send_wait_id(self, args=[], stdout=None, stderr=None, pass_fds=[]):
        with TemporaryFD("notification-id") as tmpfd:
            [fd, tmp] = tmpfd
            ns_proc = self.notify_send_proc([f"--id-fd={fd}"] + args,
                                            stdout=stdout,
                                            stderr=stderr,
                                            pass_fds=pass_fds + [fd])

            fd_value = self.wait_for_output(ns_proc, tmp)

        self.assertIsNone(ns_proc.poll())
        return [ns_proc, int(fd_value.strip())]

    def wait_for_output(self, process, io_channel, want_value=None):
        while True:
            line = io_channel.readline()
            if (not want_value and line) or (
                want_value and want_value.encode() in line) or process.poll():
                break

        try:
            process.communicate(timeout=0.5)
        except subprocess.TimeoutExpired:
            pass

        line = line.decode('utf-8')

        if want_value:
            self.assertIn(want_value, line)
        else:
            self.assertTrue(line)

        return line.strip()


class TestBaseFDONotifySend(TestBaseNotifySend):
    """Test mocking notification-daemon"""

    @classmethod
    def setUpClass(cls):
        TestBaseNotifySend.setUpClass()
        cls.caps = DEFAULT_CAPS.copy()
        cls.template = "notification_daemon"
        cls.env["NOTIFY_IGNORE_PORTAL"] = "1"

    def assertNotificationMatches(
        self,
        notification,
        exp_app_name="notify-send",
        exp_replaces_id=0,
        exp_app_icon="",
        exp_summary="",
        exp_body="",
        exp_actions=[],
        exp_hints={},
        exp_expire_timeout=-1,
    ):
        [
            app_name,
            replaces_id,
            app_icon,
            summary,
            body,
            actions,
            hints,
            expire_timeout,
        ] = notification

        self.assertEqual(app_name, exp_app_name)
        self.assertEqual(replaces_id, exp_replaces_id)
        self.assertEqual(app_icon, exp_app_icon)
        self.assertEqual(summary, exp_summary)
        self.assertEqual(body, exp_body)
        self.assertEqual(actions, exp_actions)
        self.assertEqual(hints, exp_hints)
        self.assertEqual(expire_timeout, exp_expire_timeout)


class TestNotifySend(TestBaseFDONotifySend):
    """Test mocking notification-daemon"""

    def test_no_options(self):
        """notify-send with no options"""

        ns_proc = self.notify_send(["title", "my text"])

        notification = self.assertDaemonCall("Notify")
        self.assertNotificationMatches(
            notification,
            exp_summary="title",
            exp_body="my text",
            exp_hints={
                "urgency": 1,
                "sender-pid": ns_proc.pid,
            },
        )

    def test_options(self):
        """notify-send with some options"""

        ns_proc = self.notify_send(
            [
                "--replace-id", "1234",
                "--expire-time", "27",
                "--app-name", "foo.App",
                "--app-icon", "some-app-icon",
                "--icon", "some-icon",
                "--category", "some.category",
                "--transient",
                "--hint", "string:desktop-entry:notify-send-app",
                "--urgency", "critical",
                "title",
                "my text",
            ]
        )

        notification = self.assertDaemonCall("Notify")
        self.assertNotificationMatches(
            notification,
            exp_replaces_id=1234,
            exp_app_name="foo.App",
            exp_app_icon="some-app-icon",
            exp_summary="title",
            exp_body="my text",
            exp_expire_timeout=27,
            exp_hints={
                "urgency": 2,
                "sender-pid": ns_proc.pid,
                "image_path": "some-icon",
                "category": "some.category",
                "desktop-entry": "notify-send-app",
                "transient": True,
            },
        )

    def test_image_only(self):
        """notify-send with image"""

        ns_proc = self.notify_send(["image-only", "-i", "my-image"])

        notification = self.assertDaemonCall("Notify")
        self.assertNotificationMatches(
            notification,
            exp_summary="image-only",
            exp_hints={
                "urgency": 1,
                "sender-pid": ns_proc.pid,
                "image_path": "my-image",
            },
        )

    def test_file_image_only(self):
        """notify-send with local image"""

        image_file = self.get_local_asset("applet-critical.png")
        ns_proc = self.notify_send(["image-only", "-i", image_file])

        notification = self.assertDaemonCall("Notify")
        self.assertNotificationMatches(
            notification,
            exp_summary="image-only",
            exp_hints={
                "urgency": 1,
                "sender-pid": ns_proc.pid,
                "image_path": image_file,
            },
        )

    def test_app_icon_only(self):
        """notify-send with app-icon"""

        ns_proc = self.notify_send(["app-icon-only", "-n", "my-app-icon"])

        notification = self.assertDaemonCall("Notify")
        self.assertNotificationMatches(
            notification,
            exp_summary="app-icon-only",
            exp_app_icon="my-app-icon",
            exp_hints={
                "urgency": 1,
                "sender-pid": ns_proc.pid,
            },
        )

    def test_custom_hints(self):
        """notify-send with some custom hints"""

        ns_proc = self.notify_send(
            [
                "-h", "int:int-hint:55",
                "-h", "double:double-hint:5.5",
                "-h", "byte:byte-hint:255",
                "-h", "byte:byte-hex-hint:0x11",
                "-h", "boolean:boolean-hint-true:true",
                "-h", "boolean:boolean-hint-TRUE:TRUE",
                "-h", "boolean:boolean-hint-not-true:not-true",
                "-h", "boolean:boolean-hint-false:false",
                "-h", "boolean:boolean-hint-FALSE:FALSE",
                "-h", "variant:variant-hint-dict:{'a': 1, 'b': 2}",
                "-h", "string:string-hint:string-hint-value",
                "hints!"
            ]
        )

        notification = self.assertDaemonCall("Notify")
        self.assertNotificationMatches(
            notification,
            exp_summary="hints!",
            exp_hints={
                "urgency": 1,
                "sender-pid": ns_proc.pid,
                "int-hint": 55,
                "double-hint": 5.5,
                "byte-hint": 0xff,
                "byte-hex-hint": 0x11,
                "boolean-hint-true": True,
                "boolean-hint-TRUE": True,
                "boolean-hint-not-true": False,
                "boolean-hint-false": False,
                "boolean-hint-FALSE": False,
                "variant-hint-dict": {'a': 1, 'b': 2},
                "string-hint": "string-hint-value",
            },
        )

    def test_actions_not_supported(self):
        """notify-send with action not supported"""

        ns_proc = self.notify_send_proc([
            "action!", "Chose it",
            "--print-id",
            "--action=Foo",
            "--action=Bar=bar-item",
        ], stderr=subprocess.PIPE)

        [_, stderr] = ns_proc.communicate(timeout=5)
        ns_proc.wait()

        self.assertEqual(ns_proc.returncode, 1)
        self.assertIn("Actions are not supported by this notifications server. " +
            "Displaying non-interactively", stderr.decode("utf-8"))

        notification = self.assertDaemonCall("Notify")
        self.assertNotificationMatches(
            notification,
            exp_summary="action!",
            exp_body="Chose it",
            exp_hints={
                "urgency": 1,
                "sender-pid": ns_proc.pid,
            },
        )

    def test_close_notification(self):
        [ns_proc, notification_id] =  self.notify_send_wait_id([
            "Wait me", "--wait",
        ], stdout=subprocess.PIPE)

        self.obj_daemon.EmitSignal(DBUS_IFACE, "NotificationClosed", "uu",
                                   (notification_id, 3))
        [stdout, _] = ns_proc.communicate(timeout=5)
        self.assertFalse(stdout.decode("utf-8").strip())
        self.assertEqual(ns_proc.returncode, 0)

    def test_sigint_while_notifying(self):
        [ns_proc, _] =  self.notify_send_wait_id([
            "Wait and cancel me", "--wait",
        ], stderr=subprocess.PIPE)

        ns_proc.send_signal(signal.SIGINT)
        [_, stderr] = ns_proc.communicate()

        self.assertEqual(ns_proc.returncode, 0)
        self.assertIn("Wait cancelled, closing notification",
                      stderr.decode("utf-8"))


class TestNotifySendActions(TestBaseFDONotifySend):
    """Test mocking notification-daemon with actions"""

    @classmethod
    def setUpClass(cls):
        TestBaseFDONotifySend.setUpClass()
        cls.caps.append("actions")

    def show_actions_notification(self, actions, stdout=subprocess.PIPE, stderr=None,
                                  selected_action_fd=None, activation_token_fd=None):
        args = ["action!", "Choose it"]

        exp_actions = []
        for action in actions:
            label = action[0]
            if len(action) > 1:
                action_id = action[1]
                args.append(f"--action={action_id}={label}")
            else:
                action_id = str(int(len(exp_actions) / 2))
                args.append(f"--action={label}")

            if action_id in exp_actions:
                action_idx = exp_actions.index(action_id)
                del exp_actions[action_idx + 1]
                exp_actions.insert(action_idx + 1, label)
                continue

            exp_actions.append(action_id)
            exp_actions.append(label)

        pass_fds=[]
        if selected_action_fd:
            args.append(f"--selected-action-fd={selected_action_fd}")
            pass_fds.append(selected_action_fd)
        if activation_token_fd:
            args.append(f"--activation-token-fd={activation_token_fd}")
            pass_fds.append(activation_token_fd)

        [ns_proc, notification_id] = self.notify_send_wait_id(args,
                                                              stdout=stdout,
                                                              stderr=stderr,
                                                              pass_fds=pass_fds)

        notification = self.assertDaemonCall("Notify")
        self.assertNotificationMatches(
            notification,
            exp_summary="action!",
            exp_body="Choose it",
            exp_actions=exp_actions,
            exp_hints={
                "urgency": 1,
                "sender-pid": ns_proc.pid,
            },
        )

        return [ns_proc, notification_id]

    def check_activate_action(self, action_id, actions=[("Foo",), ("Bar", "bar-action")],
                              activation_token="activation-token"):
        with TemporaryFD("action-id") as tmpfd:
            [action_fd, action_tmp] = tmpfd
            with TemporaryFD("token-id") as tmptokenfd:
                [token_fd, token_tmp] = tmptokenfd
                [ns_proc, notification_id] = self.show_actions_notification(actions,
                                                                            selected_action_fd=action_fd,
                                                                            activation_token_fd=token_fd)
                if activation_token:
                    activation_token += f"-{action_fd}-{token_fd}"
                    self.obj_daemon.EmitSignal(DBUS_IFACE, "ActivationToken", "us",
                                               (notification_id, activation_token))

                action_id = str(action_id)
                self.obj_daemon.EmitSignal(DBUS_IFACE, "ActionInvoked", "us",
                                           (notification_id, action_id))

                action_value = self.wait_for_output(ns_proc, action_tmp)
                token_value = self.wait_for_output(ns_proc, token_tmp)

        [stdout, _] = ns_proc.communicate(timeout=5)
        self.assertEqual(action_value, action_id)
        self.assertIn(action_id, stdout.decode('utf-8'))
        self.assertEqual(ns_proc.returncode, 0)

        if activation_token:
            self.assertEqual(token_value, activation_token)
        else:
            self.assertFalse(token_value)

    def test_activate_numeric_action(self):
        """notify-send with action"""
        self.check_activate_action(0)

    def test_activate_named_action(self):
        """notify-send with action"""
        self.check_activate_action("bar-action")

    def test_activate_third_unnamed_action(self):
        """notify-send with action"""
        self.check_activate_action(action_id=2, actions=[
            ("foo",), ("default", "id"), ("baz",)])

    def test_activate_replaced_action(self):
        """notify-send with action"""
        self.check_activate_action(action_id="foo-action", actions=[
            ("Foo", "foo-action"), ("FooBar", "foo-action")])

    def test_activate_invalid_action(self):
        with TemporaryFD("action-id") as tmpfd:
            [fd, tmp] = tmpfd
            [ns_proc, notification_id] = self.show_actions_notification([("Foo", "foo")],
                                                                        stderr=subprocess.PIPE,
                                                                        selected_action_fd=fd)
            action_id = "bar"
            self.obj_daemon.EmitSignal(DBUS_IFACE, "ActionInvoked", "us",
                                       (notification_id, action_id))

            self.wait_for_output(ns_proc, ns_proc.stderr,
                                 f"Received unknown action {action_id}")

            ns_proc.send_signal(signal.SIGINT)
            [stdout, stderr] = ns_proc.communicate(timeout=5)
            self.assertNotIn(action_id, stdout.decode("utf-8").strip())
            self.assertIn("Wait cancelled, closing notification",
                          stderr.decode("utf-8").strip())
            self.assertEqual(ns_proc.returncode, 0)

            self.assertFalse(tmp.readlines())

    def test_close_notification(self):
        [ns_proc, notification_id] = self.show_actions_notification([("Foo",)])

        self.obj_daemon.EmitSignal(DBUS_IFACE, "NotificationClosed", "uu",
                                   (notification_id, 2))
        [stdout, _] = ns_proc.communicate(timeout=5)
        self.assertFalse(stdout.decode("utf-8").strip())
        self.assertEqual(ns_proc.returncode, 0)


class TestBasePortalNotifySend(TestBaseNotifySend):
    """Test notify-send with portal"""

    def assertNotificationMatches(
        self,
        args,
        exp_id=0,
        exp_notification={},
    ):
        [
            notification_id,
            notification,
        ] = args

        self.assertEqual(notification_id, exp_id)
        self.assertEqual(notification, exp_notification)

    @classmethod
    def setUpClass(cls):
        TestBaseNotifySend.setUpClass()
        cls.caps = DEFAULT_CAPS.copy()
        cls.template = cls.get_local_asset("portal_template.py")
        cls.env["NOTIFY_FORCE_PORTAL"] = "1"


class TestPortalNotifySend(TestBasePortalNotifySend):
    """Test notify-send with portal"""

    def test_no_options(self):
        """notify-send with no options"""

        self.notify_send(["title", "my text"])

        notification = self.assertDaemonCall("AddNotification")
        self.assertNotificationMatches(
            notification,
            exp_id="libnotify-flatpak.(null)-notify-send-1",
            exp_notification={
                "title": "title",
                "body": "my text",
                "priority": "normal",
            },
        )

    def test_options(self):
        """notify-send with some options"""

        self.notify_send(
            [
                "--replace-id", "1234",
                "--expire-time", "27",
                "--app-name", "foo.App",
                "--app-icon", "some-app-icon",
                "--icon", "some-icon",
                "--category", "some.category",
                "--transient",
                "--hint", "string:desktop-entry:notify-send-app",
                "--urgency", "critical",
                "title",
                "my text",
            ]
        )

        notification = self.assertDaemonCall("AddNotification")
        self.assertNotificationMatches(
            notification,
            exp_id="libnotify-flatpak.(null)-notify-send-1234",
            exp_notification={
                "title": "title",
                "body": "my text",
                "priority": "urgent",
                "icon": ("themed", (["some-icon", "some-icon-symbolic"])),
            },
        )

    def test_image_only(self):
        """notify-send with image"""

        self.notify_send(["image-only", "-i", "my-image"])

        notification = self.assertDaemonCall("AddNotification")
        self.assertNotificationMatches(
            notification,
            exp_id="libnotify-flatpak.(null)-notify-send-1",
            exp_notification={
                "title": "image-only",
                "body": "",
                "priority": "normal",
                "icon": ("themed", (["my-image", "my-image-symbolic"])),
            },
        )

    def test_file_image_only(self):
        """notify-send with local image"""

        image_path = self.get_local_asset("applet-critical.png")
        self.notify_send(["image-path-only", "-i", image_path])

        with open(image_path, 'rb') as image_file:
            image_bytes = image_file.read()
            notification = self.assertDaemonCall("AddNotification")
            self.assertNotificationMatches(
                notification,
                exp_id="libnotify-flatpak.(null)-notify-send-1",
                exp_notification={
                    "title": "image-path-only",
                    "body": "",
                    "priority": "normal",
                    "icon": ("bytes", dbus.Array(image_bytes, signature="y")),
                },
            )

    def test_app_icon_only(self):
        """notify-send with app-icon"""

        self.notify_send(["app-icon-only", "-n", "my-app-icon"])

        notification = self.assertDaemonCall("AddNotification")
        self.assertNotificationMatches(
            notification,
            exp_id="libnotify-flatpak.(null)-notify-send-1",
            exp_notification={
                "title": "app-icon-only",
                "body": "",
                "priority": "normal",
            },
        )

    def test_custom_hints(self):
        """notify-send with some custom hints"""

        self.notify_send(
            [
                "-h", "int:int-hint:55",
                "-h", "double:double-hint:5.5",
                "-h", "byte:byte-hint:255",
                "-h", "byte:byte-hex-hint:0x11",
                "-h", "boolean:boolean-hint-true:true",
                "-h", "boolean:boolean-hint-TRUE:TRUE",
                "-h", "boolean:boolean-hint-not-true:not-true",
                "-h", "boolean:boolean-hint-false:false",
                "-h", "boolean:boolean-hint-FALSE:FALSE",
                "-h", "variant:variant-hint-dict:{'a': 1, 'b': 2}",
                "-h", "string:string-hint:string-hint-value",
                "hints!"
            ]
        )

        notification = self.assertDaemonCall("AddNotification")
        self.assertNotificationMatches(
            notification,
            exp_id="libnotify-flatpak.(null)-notify-send-1",
            exp_notification={
                "title": "hints!",
                "body": "",
                "priority": "normal",
            },
        )


class TestPortalNotifySendActions(TestBasePortalNotifySend):
    """Test notify-send using portal with actions"""

    def show_actions_notification(self, actions, stdout=subprocess.PIPE, stderr=None,
                                  selected_action_fd=None, activation_token_fd=None):
        args = ["action!", "Choose it"]

        exp_actions = []
        for action in actions:
            label = action[0]
            if len(action) > 1:
                action_id = action[1]
                args.append(f"--action={action_id}={label}")
            else:
                action_id = str(int(len(exp_actions)))
                args.append(f"--action={label}")

            other = [a for a in exp_actions if a["action"] == action_id]
            if other:
                other[0]["label"] = label
                continue

            exp_actions.append({"label": label, "action": action_id})

        pass_fds=[]
        if selected_action_fd:
            args.append(f"--selected-action-fd={selected_action_fd}")
            pass_fds.append(selected_action_fd)
        if activation_token_fd:
            args.append(f"--activation-token-fd={activation_token_fd}")
            pass_fds.append(activation_token_fd)

        [ns_proc, _] = self.notify_send_wait_id(args, stdout=stdout, stderr=stderr,
                                                pass_fds=pass_fds)

        notification = self.assertDaemonCall("AddNotification")

        self.assertNotificationMatches(
            notification,
            exp_id="libnotify-flatpak.(null)-notify-send-1",
            exp_notification={
                "title": "action!",
                "body": "Choose it",
                "priority": "normal",
                "buttons": dbus.Array(exp_actions, signature="a{sv}"),
            },
        )

        return [ns_proc, notification[0]]

    def check_activate_action(self, action_id, actions=[("Foo",), ("Bar", "bar-action")]):
        with TemporaryFD("action-id") as tmpfd:
            [fd, tmp] = tmpfd
            [ns_proc, notification_id] = self.show_actions_notification(actions,
                                                                        selected_action_fd=fd)
            action_id = str(action_id)
            self.obj_daemon.EmitActionInvoked(notification_id, action_id,
                                              dbus.Array((), signature="v"))

            fd_value = self.wait_for_output(ns_proc, tmp)

        [stdout, _] = ns_proc.communicate(timeout=5)
        self.assertEqual(fd_value, action_id)
        self.assertIn(action_id, stdout.decode('utf-8'))
        self.assertEqual(ns_proc.returncode, 0)

    def test_activate_numeric_action(self):
        """notify-send with action"""
        self.check_activate_action(0)

    def test_activate_named_action(self):
        """notify-send with action"""
        self.check_activate_action("bar-action")

    def test_activate_third_unnamed_action(self):
        """notify-send with action"""
        self.check_activate_action(action_id=2, actions=[
            ("foo",), ("default", "id"), ("baz",)])

    def test_activate_replaced_action(self):
        """notify-send with action"""
        self.check_activate_action(action_id="foo-action", actions=[
            ("Foo", "foo-action"), ("FooBar", "foo-action")])

    def test_activate_invalid_action(self):
        with TemporaryFD("action-id") as tmpfd:
            [fd, tmp] = tmpfd

            [ns_proc, notification_id] = self.show_actions_notification([("Foo", "foo")],
                                                                        stderr=subprocess.PIPE,
                                                                        selected_action_fd=fd)

            action_id = "bar"
            self.obj_daemon.EmitActionInvoked(notification_id, action_id,
                                            dbus.Array((), signature="v"))

            self.wait_for_output(ns_proc, ns_proc.stderr,
                                 f"Received unknown action {action_id}")

            ns_proc.send_signal(signal.SIGINT)
            [stdout, _] = ns_proc.communicate(timeout=5)
            self.assertNotIn(action_id, stdout.decode("utf-8").strip())
            self.assertEqual(ns_proc.returncode, 0)

            self.assertFalse(tmp.readlines())


if __name__ == "__main__":
    # avoid writing to stderr
    unittest.main(testRunner=unittest.TextTestRunner(stream=sys.stdout))
