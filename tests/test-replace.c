#include <libnotify/notify.h>
#include <stdio.h>
#include <unistd.h>
#include <glib.h>

int main() {
    NotifyNotification *n;
    GError *error;
    error = NULL;

    g_type_init ();

    notify_init("Replace Test");
   
    n = notify_notification_new ("Summary", "First message", 
                                  NULL,  //no icon
                                  NULL); //don't attach to widget

   
    notify_notification_set_timeout (n, 0); //don't timeout
    
    if (!notify_notification_show (n, &error)) {
        fprintf(stderr, "failed to send notification: %s\n", error->message);
        g_error_free (error);
        return 1;
    }

    sleep(3);

    notify_notification_update (n, "Second Summary", 
                                "First mesage was replaced", NULL); 
    notify_notification_set_timeout (n, NOTIFY_EXPIRES_DEFAULT);

    if (!notify_notification_show (n, &error)) {
        fprintf(stderr, "failed to send notification: %s\n", error->message);
        g_error_free (error);
        return 1;
    }

    return 0;
}
