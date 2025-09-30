# SPDX-License-Identifier: LGPL-2.1-or-later

import logging

import dbus.service
from dbusmock import MOCK_IFACE


BUS_NAME = "org.freedesktop.portal.Desktop"
MAIN_OBJ = "/org/freedesktop/portal/desktop"
MAIN_IFACE = "org.freedesktop.portal.Notification"
SYSTEM_BUS = False
VERSION = 1


logger = logging.getLogger("portal")
logger.setLevel(logging.DEBUG)


def load(mock, parameters={}):
    logger.debug(f"Loading parameters: {parameters}")

    mock.AddProperties(
        MAIN_IFACE,
        dbus.Dictionary(
            {
                "version": dbus.UInt32(parameters.get("version", VERSION)),
                "SupportedOptions": dbus.Dictionary(
                    parameters.get("SupportedOptions", {}), signature="sv"
                ),
            },
        ),
    )
    mock.notifications = {}


@dbus.service.method(
    MAIN_IFACE,
    in_signature="sa{sv}",
    out_signature="",
)
def AddNotification(self, id, notification):
    logger.debug(f"AddNotification({id}, {notification})")

    self.notifications[id] = notification


@dbus.service.method(
    MAIN_IFACE,
    in_signature="s",
    out_signature="",
)
def RemoveNotification(self, id):
    logger.debug(f"AddNotification({id})")

    del self.notifications[id]


@dbus.service.method(
    MOCK_IFACE,
    in_signature="ssav",
    out_signature="",
)
def EmitActionInvoked(self, id, action, parameter):
    logger.debug(f"EmitActionInvoked({id}, {action}, {parameter})")

    self.EmitSignal(
        MAIN_IFACE,
        "ActionInvoked",
        "ssav",
        [
            id,
            action,
            dbus.Array(parameter, signature="v"),
        ],
    )
