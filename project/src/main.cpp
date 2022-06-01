#include <Arduino.h>

// Pin definition
const int CLOCK_pin = 11;
const int LOAD_pin = 12;
const int DIN_pin = 13;
const int controller_x_axis = A0;
const int controller_y_axis = A1;

// MAX7219 Register Addresses
const byte max7219_REG_noop = 0x00;
const byte max7219_REG_decodeMode = 0x09;
const byte max7219_REG_intensity = 0x0a;
const byte max7219_REG_scanLimit = 0x0b;
const byte max7219_REG_shutdown = 0x0c;
const byte max7219_REG_displayTest = 0x0f;

// Constants
const int modules = 4;
const int game_speed = 100; // Miliseconds
const int array_size = 64 * modules;

// Send 8 bits
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

// Init 16 bit
void init(byte reg_addr, byte reg_data)
{
  digitalWrite(LOAD_pin, LOW);
  for (int i = 0; i < modules; i++)
  {
    SendByte(reg_addr);
    SendByte(reg_data);
  }
  digitalWrite(LOAD_pin, HIGH);
}

// Display matrix
char matrix[8][modules];

// Draw display matrix
void draw_matrix()
{

  byte mask = 0;
  for (byte i = 0; i < 8; i++)
  {
    digitalWrite(LOAD_pin, LOW);

    for (byte j = 1; j <= modules; j++)
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

// Clear screen and set display matrix all 0s
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

struct Position
{
  byte row;
  byte module;
  byte column;
};

Position current_position;

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

// Returns random position
Position get_random_positon()
{
  Position random_position;
  randomSeed(analogRead(A0) + analogRead(A1) + 11);
  random_position.row = random(0, 8);
  random_position.module = random(0, modules);
  random_position.column = random(0, 8);
  return random_position;
}

// Random positon on draw matrix
Position spawn_food(Position *arr, int stepper, int length)
{
  Position position_food;

  bool gefunden = false;
  while (!gefunden)
  {
    gefunden = true;
    position_food = get_random_positon();

    for (int i = stepper - length; i < stepper; i++)
    {
      if ((arr[i].row == position_food.row) && (arr[i].module == position_food.module) && (arr[i].column == position_food.column))
      {
        gefunden = false;
      }
    }
  }

  matrix[position_food.row][position_food.module] |= (0x01 << position_food.column);
  return position_food;
}

// Init new game
void new_game(Position arr[array_size])
{
  // Init position
  // Tail
  arr[0].row = 4;
  arr[0].module = 0;
  arr[0].column = (0x01 << 4);

  // Body
  arr[1].row = 0;
  arr[1].module = 1;
  arr[1].column = (0x01 << 3);

  // Head
  arr[2].row = 4;
  arr[2].module = 0;
  arr[2].column = (0x01 << 2);

  // Head positon is current position
  current_position.row = arr[2].row;
  current_position.module = arr[2].module;
  current_position.column = arr[2].column;

  // Reset display
  clear_screen();
  matrix[arr[0].row][arr[0].module] = arr[0].column;
  matrix[arr[1].row][arr[1].module] = arr[1].column;
  matrix[current_position.row][current_position.module] = current_position.column;
  draw_matrix();
}

void setup()
{
  Serial.begin(9600);

  // Set output pins for LED Matrix
  pinMode(DIN_pin, OUTPUT);
  pinMode(CLOCK_pin, OUTPUT);
  pinMode(LOAD_pin, OUTPUT);

  // Set input pins for controller
  pinMode(controller_x_axis, INPUT);
  pinMode(controller_y_axis, INPUT);

  // Activate alle rows
  init(max7219_REG_scanLimit, 0x07);

  // No decofing
  init(max7219_REG_decodeMode, 0x00);

  // No display test
  init(max7219_REG_displayTest, 0x00);

  // No shutdown
  init(max7219_REG_shutdown, 0x01);

  // Max brightness
  init(max7219_REG_intensity, 0x0F);
}

void loop()
{
  // Setup
  int stepper = 3;
  int length = 3;
  int direction = 2;
  bool collission_detected = false;

  // Game array
  Position arr[array_size];

  // Init new game
  new_game(arr);

  // Spawn first food
  Position position_food = spawn_food(arr, stepper, length);

  while (1)
  {
    // Game Loop
    delay(game_speed);

    // ** Processing input **
    int up_down = analogRead(controller_x_axis);    // up -> 0, down -> 1023
    int right_left = analogRead(controller_y_axis); // right -> 0, left -> 1023

    // if direction up-down
    if (direction <= 1)
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

    // ** Processing game **
    // Removes tail vom draw matrix
    matrix[arr[stepper - length].row][arr[stepper - length].module] &= ~(arr[stepper - length].column);

    // Removes tail from Game array
    arr[stepper - length].row = 0;
    arr[stepper - length].module = 0;
    arr[stepper - length].column = 0;

    // Appends current position to draw matrix
    matrix[current_position.row][current_position.module] |= current_position.column;

    // Append current position to Game array
    arr[stepper].row = current_position.row;
    arr[stepper].module = current_position.module;
    arr[stepper].column = current_position.column;

    // Check if Game array is full
    if (stepper == array_size - 1)
    {
      // Rewrites positions at the beginning of the Game array
      for (int i = length; i >= 0; i--)
      {
        arr[length - i].row = arr[array_size - 1 - i].row;
        arr[length - i].module = arr[array_size - 1 - i].module;
        arr[length - i].column = arr[array_size - 1 - i].column;
      }
      stepper = length;
    }

    // Check if current_position (head) equals position_food
    if ((current_position.row == position_food.row) && (current_position.module == position_food.module) && (current_position.column == (0x01 << position_food.column)))
    {
      // Increase snake size
      length++;

      // Spawn food
      position_food = spawn_food(arr, stepper, length);
    }

    // Check for collissions
    for (int i = stepper - length; i < stepper - 3; i++)
    {
      if ((arr[i].row == current_position.row) && (arr[i].module == current_position.module) && (arr[i].column == current_position.column))
      {
        // Game Over
        Serial.println("Game Over!");
        collission_detected = true;
        break;
      }
    }
    if (collission_detected)
    {
      Serial.println("Restarting...");
      break;
    }

    // Increase stepper and draw matix
    stepper++;
    draw_matrix();
  }
}