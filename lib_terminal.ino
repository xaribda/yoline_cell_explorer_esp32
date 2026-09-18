void doTermitalImitation() {
  // Если прилетели данные от модема — выводим на экран компьютера
  if (modemSoftwareSerial.available()) {
    while (modemSoftwareSerial.available()) {
      Serial.write(modemSoftwareSerial.read());
    }
  }

  // Если мы что-то ввели в терминале — отправляем в модем
  if (Serial.available()) {
    while (Serial.available()) {
      modemSoftwareSerial.write(Serial.read());
    }
  }

}