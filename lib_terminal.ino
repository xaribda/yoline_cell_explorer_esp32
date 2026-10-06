void doTermitalImitation() {
  // Если прилетели данные от модема — выводим на экран компьютера
  if (Serial0.available()) {
    while (Serial0.available()) {
      Serial.write(Serial0.read());
    }
  }

  // Если мы что-то ввели в терминале — отправляем в модем
  if (Serial.available()) {
    while (Serial.available()) {
      Serial0.write(Serial.read());
    }
  }

}