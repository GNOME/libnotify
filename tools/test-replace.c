#include <libnotify/notify.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    notify_init("Replace Test");
    
    NotifyHandle *n = notify_send_notification(NULL, // replaces nothing
                                               NOTIFY_URGENCY_NORMAL,
                                               "Summary", "Content",
                                               NULL, // no icon
                                               FALSE, 0, // does not expire
                                               NULL, // no user data
                                               0); // no actions

    if (!n) {
        fprintf(stderr, "failed to send notification\n");
        return 1;
    }


    sleep(3);

    notify_send_notification(n, NOTIFY_URGENCY_NORMAL, "Second Summary", "Second Content",
                             NULL, TRUE, time(NULL) + 5, NULL, 0);
    
    return 0;
}
