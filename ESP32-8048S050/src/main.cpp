#include <Arduino.h>
#include <display.h>

void setup() {
  // Serial Debug.
  Serial0.begin(115200);
  Serial0.println("Starting...");
  InitDisplay();
}
void loop() {
  // Service lvgl tasks
  DisplayTasks();
}