#include "task_toogle_boot.h"

#define BOOT 0

void Task_Toogle_BOOT(void *pvParameters)
{
    unsigned long buttonPressStartTime = 0;
    while (true)
    {
        if (digitalRead(BOOT) == LOW)
        {
            if (buttonPressStartTime == 0)
            {
                buttonPressStartTime = millis();
            }
            else if (millis() - buttonPressStartTime > 2000)
            {
                Delete_info_File();
                vTaskDelete(NULL);
            }
        }
        else
        {
            buttonPressStartTime = 0;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}