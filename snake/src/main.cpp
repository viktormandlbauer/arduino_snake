#include <Arduino.h>

// Pin definition
const int CLOCK_pin = 11;
const int LOAD_pin = 12;
const int DIN_pin = 13;

// MAX7219 Register Addresses
const byte max7219_REG_noop = 0x00;
const byte max7219_REG_decodeMode = 0x09;
const byte max7219_REG_intensity = 0x0a;
const byte max7219_REG_scanLimit = 0x0b;
const byte max7219_REG_shutdown = 0x0c;
const byte max7219_REG_displayTest = 0x0f;

struct Position
{
  byte row;
  byte module;
  byte column;
};

// Module in Serie
const int modules = 4;
const int array_size = 256;
Position current_position;
char matrix[8][modules];

void SendByte(byte data)
{

  byte i = 8;
  byte mask;
  while (i > 0)
  {
    mask = 0x01 << (i - 1);

    digitalWrite(CLOCK_pin, LOW);
    if (data & mask)
    {
      digitalWrite(DIN_pin, HIGH);
    }
    else
    {
      digitalWrite(DIN_pin, LOW);
    }
    digitalWrite(CLOCK_pin, HIGH);
    --i;
  }
}

void init(byte reg_addr, byte reg_data)
{
  digitalWrite(LOAD_pin, LOW);
  for (int i = 0; i < 4; i++)
  {
    SendByte(reg_addr);
    SendByte(reg_data);
  }
  digitalWrite(LOAD_pin, HIGH);
}

// Gibt das Array auf dem Display aus
void draw_matrix()
{

  byte mask = 0;
  for (byte i = 0; i < 8; i++)
  {
    digitalWrite(LOAD_pin, LOW);

    for (byte j = 1; j <= 4; j++)
    {
      // Row
      SendByte(i + 1);
      for (byte k = 0; k < 8; k++)
      {
        mask = 0x01 << k;
        digitalWrite(CLOCK_pin, LOW);

        // Column
        if (mask & matrix[i][modules - j])
        {
          digitalWrite(DIN_pin, HIGH);
        }
        else
        {
          digitalWrite(DIN_pin, LOW);
        }

        digitalWrite(CLOCK_pin, HIGH);
      }
    }
    digitalWrite(LOAD_pin, HIGH);
  }
}

void up()
{
  if (current_position.row == 7)
  {
    current_position.row = 0;
  }
  else
  {
    current_position.row = current_position.row + 1;
  }
}

void down()
{
  if (current_position.row == 0)
  {
    current_position.row = 7;
  }
  else
  {
    current_position.row = current_position.row - 1;
  }
}

void right()
{
  if (current_position.column == 0x01)
  {
    if (current_position.module == modules - 1)
    {
      current_position.column = 0x80;
      current_position.module = 0;
    }
    else
    {
      current_position.column = 0x80;
      current_position.module = current_position.module + 1;
    }
  }
  else
  {
    current_position.column = current_position.column >> 1;
  }
}

void left()
{
  if (current_position.column == 0x80)
  {
    if (current_position.module == 0)
    {
      current_position.column = 0x01;
      current_position.module = modules - 1;
    }
    else
    {
      current_position.column = 0x01;
      current_position.module = current_position.module - 1;
    }
  }
  else
  {
    current_position.column = current_position.column << 1;
  }
}

void process_joystick_input(){

}

void clear_screen()
{
  init(1, 0);
  init(2, 0);
  init(3, 0);
  init(4, 0);
  init(5, 0);
  init(6, 0);
  init(7, 0);
  init(8, 0);

  for (byte i = 0; i < 8; i++)
  {
    for (byte j = 0; j < modules; j++)
    {
      matrix[i][j] = 0;
    }
  }
}

// Gibt eine zufällige Position auf einem Module zurück
Position get_random_positon()
{
  Position random_position;
  randomSeed(analogRead(A0) + analogRead(A1) + 11);
  random_position.row = random(0, 8);
  random_position.module = random(0, modules);
  random_position.column = random(0, 8);
  return random_position;
}

// Erstellt einen zufälligen Punkt auf der Matrix
Position spawn_food()
{
  Position position_food = get_random_positon();
  matrix[position_food.row][position_food.module] |= (0x01 << position_food.column);
  draw_matrix();
  return position_food;
}

// ** Erstellt eine neues Spiel **
void new_game(Position arr[array_size])
{
  // Startpositon auf Spielfeld wird bestimmt
  arr[0].row = 4;
  arr[0].module = 1;
  arr[0].column = (0x01 << 4);

  arr[1].row = 4;
  arr[1].module = 1;
  arr[1].column = (0x01 << 3);

  arr[2].row = 4;
  arr[2].module = 1;
  arr[2].column = (0x01 << 2);

  current_position.row = arr[2].row;
  current_position.module = arr[2].module;
  current_position.column = arr[2].column;

  // Zeichenmatrix wird zurückgesetzt und neu erstellt
  clear_screen();
  matrix[arr[0].row][arr[0].module] = arr[0].column;
  matrix[arr[1].row][arr[1].module] = arr[1].column;
  matrix[current_position.row][current_position.module] = current_position.column;
  draw_matrix();
}

void setup()
{
  Serial.begin(9600);

  // Set Output Pins
  pinMode(DIN_pin, OUTPUT);
  pinMode(CLOCK_pin, OUTPUT);
  pinMode(LOAD_pin, OUTPUT);

  // Set Input Pins
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);

  // Aktiviert alle Reihen
  init(max7219_REG_scanLimit, 0x07); // set to scan all row

  // Kein decoding
  init(max7219_REG_decodeMode, 0x00);

  // Kein shutdown
  init(max7219_REG_displayTest, 0x00);

  // Kein shutdown mode
  init(max7219_REG_shutdown, 0x01);

  // Max brightness
  init(max7219_REG_intensity, 0x0F);

  Serial.write("\nStarting Snake");
}

void loop()
{

  int stepper = 3;
  int length = 3;
  int direction = 2;
  bool collission_detected = false;

  Position arr[array_size];

  new_game(arr);

  Position position_food = spawn_food();

  while (1)
  {
    // Game Loop

    // Serial print current position
    // char out[256];
    // snprintf(out, 256, "\nCurrent position:\n\tRow\t->\t%d\n\tModule\t->\t%d\n\tColumn\t->\t%d", current_position.row, current_position.module, current_position.column);
    // Serial.print(out);

    // ** Joystick analog read **
    // up -> 0, down -> 1023
    int up_down = analogRead(A0);

    // right -> 0, left -> 1023
    int right_left = analogRead(A1);

    // ** Joystick logic **
    // if direction up-down
    if (direction < 1)
    {
      // only right-left possible
      if (right_left < 400)
      {
        right();
        direction = 2;
      }
      else if (right_left > 600)
      {
        left();
        direction = 3;
      }
      else
      {
        // Switch-case for keeping direction
        switch (direction)
        {
        case 0:
          up();
          break;
        case 1:
          down();
          break;
        }
      }
    }
    // else direction right-left
    else
    {
      // only up-down possible
      if (up_down < 400)
      {
        up();
        direction = 0;
      }
      else if (up_down > 600)
      {
        down();
        direction = 1;
      }
      else
      {
        // Switch-case for keeping direction
        switch (direction)
        {
        case 2:
          right();
          break;
        case 3:
          left();
          break;
        }
      }
    }

    // Check if current_position (head) equals position_food
    if ((current_position.row == position_food.row) && (current_position.module == position_food.module) && (current_position.column == (0x01 << position_food.column)))
    {
      // Increase snake size
      length++;

      // Remove food from drawmatrix and spawn_food
      matrix[position_food.row][position_food.module] &= ~(0x01 << position_food.column);
      position_food = spawn_food();
    }

    // Check for collissions
    for (int i = 0; i < stepper; i++)
    {
      if ((arr[i].row == current_position.row) && (arr[i].module == current_position.module) && (arr[i].column == current_position.column))
      {
        collission_detected = true;
        // Game Over
        break;
      }
    }

    // Appends current position for current step
    matrix[current_position.row][current_position.module] |= current_position.column;

    arr[stepper].row = current_position.row;
    arr[stepper].module = current_position.module;
    arr[stepper].column = current_position.column;

    // Removes tail from draw matrix (XOR)
    matrix[arr[stepper - length].row][arr[stepper - length].module] &= ~(arr[stepper - length].column);

    // Removes tail from field
    arr[stepper - length].row = 0;
    arr[stepper - length].module = 0;
    arr[stepper - length].column = 0;

    // Check if the array is full
    if (stepper == array_size - 1)
    {
      // Rewrites positions at the beginning of the array
      for (int i = length; i >= 0; i--)
      {
        arr[length - i].row = arr[array_size - 1 - i].row;
        arr[length - i].module = arr[array_size - 1 - i].module;
        arr[length - i].column = arr[array_size - 1 - i].column;
      }
      stepper = length;
    }

    // Increase stepper and draw matix
    stepper++;
    draw_matrix();
  }
}
