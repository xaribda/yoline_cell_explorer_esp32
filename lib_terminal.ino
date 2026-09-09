void doTermitalImitation() {
  // Если прилетели данные от модема — выводим на экран компьютера
  if (SerialAT.available()) {
    while (SerialAT.available()) {
      Serial.write(SerialAT.read());
    }
  }

  // Если мы что-то ввели в терминале — отправляем в модем
  if (Serial.available()) {
    while (Serial.available()) {
      SerialAT.write(Serial.read());
    }
  }

}