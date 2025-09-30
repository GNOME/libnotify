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

import dbusmock
import fcntl
import os
import shutil
import subprocess
import sys
import unittest

notify_send = os.environ.get("NOTIFY_SEND", None)

if not notify_send:
    notify_send = shutil.which("notify-send")

subprocess.check_output([notify_send, "--version"], text=True)


class TestNotifySend(dbusmock.DBusTestCase):
    """Test mocking notification-daemon"""

    @classmethod
    def setUpClass(cls):
        cls.start_session_bus()
        cls.dbus_con = cls.get_dbus(False)

    @classmethod
    def get_local_asset(self, *args):
        path = os.path.join(os.path.dirname(__file__), *args)
        self.assertTrue(os.path.exists(path), f"{path} does not exist")
        return path

    def setUp(self):
        (self.p_mock, self.obj_daemon) = self.spawn_server_template(
            "notification_daemon", {}, stdout=subprocess.PIPE
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

    def notify_send_proc(self, args=[], stdout=None, stderr=None):
        ns_proc = subprocess.Popen([notify_send] + args,
                                   stdout=stdout, stderr=stderr)
        return ns_proc

    def notify_send(self, args=[]):
        ns_proc = self.notify_send_proc(args)
        ns_proc.wait()
        self.assertEqual(ns_proc.returncode, 0)
        return ns_proc

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

        ns_proc = subprocess.Popen([notify_send, "image-only", "-i", "my-image"])
        ns_proc.wait()
        self.assertEqual(ns_proc.returncode, 0)

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

        ns_proc = subprocess.Popen([notify_send, "app-icon-only", "-n", "my-app-icon"])
        ns_proc.wait()
        self.assertEqual(ns_proc.returncode, 0)

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

        [_, stderr] = ns_proc.communicate()
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


if __name__ == "__main__":
    # avoid writing to stderr
    unittest.main(testRunner=unittest.TextTestRunner(stream=sys.stdout))
