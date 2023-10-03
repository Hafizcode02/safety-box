#include <LiquidCrystal_I2C.h> // Library LCD
#include <Keypad.h> // // Keypad Library
#include <ThreeWire.h> // Examples from RTCDS1302
#include <RtcDS1302.h> // Examples from RTCDS1302
#include <SPI.h> // Examples from SD
#include <SD.h> // Examples from SD

LiquidCrystal_I2C lcd(0x27, 16, 2); // Set Library LCD 16 x 2

// Konfigurasi RTCDS1302
ThreeWire myWire(A2, A1, A3); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);
// Akhir Konfigurasi RTCDS1302

#define RELAY_PIN A0 // Relay Pin untuk Doorlock

// Mendefinisikan Kode Nada Untuk Bunyi Buzzer
#define PIEZO   8 // Pin Buzzer
#define NOTE_C5  523
#define NOTE_G5  784
#define NOTE_C6  1047

// Set Up Nada Kunci Buzzer
int OpenMelody[] = {NOTE_G5, NOTE_C6};
int OpenNoteDurations[] = {12, 8};

int CloseMelody[] = {NOTE_C6, NOTE_G5};
int CloseNoteDurations[] = {12, 8};

#define playOpenMelody() playMelody(OpenMelody, OpenNoteDurations, 2)
#define playCloseMelody() playMelody(CloseMelody, CloseNoteDurations, 2)

#define passwordLenght 7 // Variable untuk menampung password yang berupa 6 character + 1 NULL

// Mendefinisikan Struct untuk menampung data agar lebih terstruktur
struct ACCOUNT {
  char user[10];
  char pass[passwordLenght];
};

char typedPassword[passwordLenght]; // 6 adalah jumlah password yang dapat di input di variable typedPassword ( 7 dengan null )
unsigned int indexUserLoggedIn;

// List Akun Pengguna
ACCOUNT acc[3] = {
  {"Hafiz", "123456"},
  {"Fitri", "654321"},
  {"Rizki", "987654"}
};

byte countClickedButton = 0; // Variable untuk menyimpan sementara berapa kali tombol telah ditekan pada keypad
char customKey; // Variable untuk menyimpan data kunci sementara
bool door = true; // Status Pintu

// Memetakan Tombol pada Keypad
const char keys [4] [4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Mengeset Pin-Pin yang terhubung ke tombol tombol keypad
const byte rowPins[4] = {7, 6, 5, 4};
const byte colPins[4] = {3, 2, 1, 0};

// Setup keypad berdasarkan konfigurasi yang sebelumnya telah dilakukan
Keypad myKeypad = Keypad(makeKeymap(keys), rowPins, colPins, 4, 4 );

void setup() {
  pinMode(PIEZO, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  doorLocked();
  rtcSetup();
  SD.begin(10);

  lcd.begin();
  lcd.backlight();
  lcd.print("   KELOMPOK 2  ");
  lcd.setCursor(0, 1);
  lcd.print("    SAFE BOX   ");
  delay(3000);
  lcd.clear();
}

void loop() {
  RtcDateTime now = Rtc.GetDateTime();

  if (door == 0)
  {
    customKey = myKeypad.getKey(); // Mengambil nilai tombol yang ditekan pada keypad

    if (customKey == '#')
    {
      lcd.clear();
      doorLocked();
      lcd.print(" Closing...");
      writingLogToSD(acc[indexUserLoggedIn].user, " Closing ", printDateTime(now));
      playCloseMelody();
      delay(1000);
      door = 1;
    }
  }

  else Open();
}

void clearData()
{
  while (countClickedButton != 0)
  { // Ini bisa digunakan untuk semua ukuran array,
    typedPassword[countClickedButton--] = 0; // bersihkan data pada array untuk data baru
  }
}

void doorLocked()
{
  digitalWrite(RELAY_PIN, LOW);
  delay(1000);
}

void doorUnlocked()
{
  digitalWrite(RELAY_PIN, HIGH);
  delay(1000);
}

void Open()
{
  RtcDateTime now = Rtc.GetDateTime(); // Mengambil data sekarang

  lcd.setCursor(0, 0);
  lcd.print(" Enter Password ");

  customKey = myKeypad.getKey();
  if (customKey)                  // memastikan keypad telah ditekan (customKey != NO_KEY)
  {
    typedPassword[countClickedButton] = customKey; // menyimpan data keypad yang ditekan kepada array
    lcd.setCursor(countClickedButton, 1); // berpindah kursor ketika penekanan tombol berikutnya
    lcd.print(typedPassword[countClickedButton]);  // mencetak nilai yang telah ditekan pada keypad dengan mengambil data pada array
    countClickedButton++;                 // menambah data indeks array +1, sekaligus melacak jumlah char setiap kali ada penginputan
  }

  if (countClickedButton == passwordLenght - 1) // jika array index sama dengan panjang password yang telah ditetapkan
  {
    for (int i = 0; i < 3; i++) { // lakukan perulangan untuk pengecekan data
      if (!strcmp(typedPassword, acc[i].pass)) { // mengecek apakah data password yang diinput sama dengan yang telah diset sebelumnya
        lcd.clear();
        doorUnlocked();
        lcd.print(" Access Granted ");
        playOpenMelody();
        delay(1000);
        lcd.clear();
        lcd.print(" Welcome, ");
        lcd.print(acc[i].user);
        door = 0;
        indexUserLoggedIn = i;
        writingLogToSD(acc[i].user, " Opening ", printDateTime(now));
        break;
      }
    }

    if (door != 0) { // kondisi ketika salah menginput password
      lcd.clear();
      lcd.print(" Access Denied ");
      playCloseMelody();
      delay(1000);
      door = 1;
    }
    clearData(); // bersihkan layar dan array
  }
}

// RTC Function
void rtcSetup() {
  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

  if (!Rtc.IsDateTimeValid())
  {
    // Common Causes:
    //    1) first time you ran and the device wasn't running yet
    //    2) the battery on the device is low or even missing
    Rtc.SetDateTime(compiled);
  }

  if (Rtc.GetIsWriteProtected())
  {
    Rtc.SetIsWriteProtected(false);
  }

  if (!Rtc.GetIsRunning())
  {
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled)
  {
    Rtc.SetDateTime(compiled);
  }
}

#define countof(a) (sizeof(a) / sizeof(a[0]))

String printDateTime(const RtcDateTime& dt)
{
  char datestring[20];

  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
             dt.Month(),
             dt.Day(),
             dt.Year(),
             dt.Hour(),
             dt.Minute(),
             dt.Second() );
  return datestring;
}
// End RTC Function

void writingLogToSD(String fullName, String action, String timeStamp)
{
  File myFile;
  myFile = SD.open("datalog.txt", FILE_WRITE); // membuka file datalog.txt

  // if the file opened okay, write to it:
  if (myFile) {
    // mencetak log ke dalam file
    myFile.print(fullName);
    myFile.print("  ");
    myFile.print(action);
    myFile.print("  ");
    myFile.print(timeStamp);
    myFile.println();

    // close the file:
    myFile.close();
  }
}

void playMelody(int *melody, int *noteDurations, int notesLength) // fungsi untuk mengatur melody nada (melodi, durasi, panjang nada)
{
  for (int thisNote = 0; thisNote < notesLength; thisNote++) {
    int noteDuration = 1000 / noteDurations[thisNote]; // menghitung durasi nada
    tone(PIEZO, melody[thisNote], noteDuration); // tone (pin buzzer, frekuensi nada, durasi)
    int pauseBetweenNotes = noteDuration * 1.30; // menghitung delay
    delay(pauseBetweenNotes);
    noTone(PIEZO); // matikan nada
  }
}
