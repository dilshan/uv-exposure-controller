//-----------------------------------------------------------------------
// Copyright 2020 Dilshan R Jayakody [jayakody2000lk@gmail.com]
//
// Permission is hereby granted, free of charge, to any person obtaining 
// a copy of this software and associated documentation files 
// (the "Software"), to deal in the Software without restriction, 
// including without limitation the rights to use, copy, modify, merge, 
// publish, distribute, sublicense, and/or sell copies of the Software, 
// and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be 
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//-----------------------------------------------------------------------

#include <avr/io.h>

#include "main.h"

#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>

typedef enum SytemState 
{
    IdleState,  // System is in idle state (waiting for action).
    EditState,  // Digit edit state.
    RunState,   // UV timer execute state.
    PauseState  // Timer pause state due to external interrupt.
} SytemState;

SytemState currentState;

uint8_t timerCounter, editDigit;
uint8_t buzzerCounter;
int16_t time, targetTime;

int main()
{
    // Stop Timer 1 and WDT.
    cli();
    wdt_disable();

    TCCR1B = 0x00;
    
    uint8_t lastSwitchState, tempDigit;
    currentState = IdleState;

    // Setup I/O ports.

    // A0 -> Run indicator.
    // A1 -> Relay driver.
    DDRA = 0x03;
    PORTA = 0x00;

    // B0 -> 74HC595 : SER (Serial Input).
    // B1 -> 74HC595 : SRCLK (Shift Register Clock).
    // B2 -> 74HC595 : RCLK (Register Clock).
    // B3 -> Digit 1 driver.
    // B4 -> Digit 2 driver.
    // B5 -> Digit 3 driver.
    // B7 -> 4011 : Buzzer.
    DDRB = 0xFF;
    PORTB = 0x00;

    // D0 -> UP / START button input.
    // D1 -> DOWN / STOP button input.
    // D2 -> SET button input.
    // D3 -> Lid open limit switch.
    DDRD = 0x00;
    PORTD = 0xFF;  

    // Wait to avoid startup spikes.
    _delay_ms(1000);
     
    editDigit = 0;
    timerCounter = 0;
    targetTime = time = MINIMUM_TIME;
    buzzerCounter = 0;

    // Start Timer 1 with 2Hz output.
    TCNT1 = 0;
    OCR1A = 15624;
    
    TCCR1A = 0x00;
    TCCR1B = 0x0C;
    TIMSK = TIMSK | 0x40;

    sei();

    lastSwitchState = PIND; 
    wdt_enable(WDTO_4S);

    // Read time setting from EEPROM and validate the limits.
    time = eeprom_read_word((uint16_t * )0);
    if((time < MINIMUM_TIME) || (time > MAXIMUM_TIME))
    {
        // Current time is out of bounds.
        time = MINIMUM_TIME;
    }

    wdt_reset();
    targetTime = time;

    // Main service loop.
    while (1)
    {
        if(((PIND & 0x01) == 0x01) && ((lastSwitchState & 0x01) == 0x00))
        {
            // Start / Increment button pressed.
            if(currentState == EditState)
            {
                // Increase the value of the current digit.
                tempDigit = extractDigit();
                tempDigit = ((tempDigit == 9) ? 0 : (tempDigit + 1));
                updateDigit(tempDigit);

                timerCounter = 0;                
            }
            else if(currentState == IdleState)
            {                
                // Start UV timer.
                time = targetTime;
                timerCounter = 0;
                PORTA = 0x03;
                currentState = RunState;
            }
        }

        if(((PIND & 0x02) == 0x02) && ((lastSwitchState & 0x02) == 0x00))
        {
            // Stop / Decrement button pressed.
            if(currentState == EditState)
            {
                // Decrease the value of the current digit.
                tempDigit = extractDigit();
                tempDigit = ((tempDigit == 0) ? 9 : (tempDigit - 1));
                updateDigit(tempDigit);

                timerCounter = 0;
            }
            else if(currentState == RunState)
            {
                // Stop UV timer.
                currentState = IdleState;
                time = targetTime;                
                PORTA = 0x00;

                // Trigger buzzer with short beep.
                timerCounter = 1;
                buzzerCounter = 2;
                PORTB = PORTB | 0x80;
            }
        }

        if(((PIND & 0x04) == 0x04) && ((lastSwitchState & 0x04) == 0x00))
        {
            // Set button pressed.
            if(currentState == IdleState)
            {
                // Enter into time edit mode.
                currentState = EditState;
                editDigit = 0;
            }
            else if(currentState == EditState)
            {
                // Move editor into next digit.
                editDigit++;

                if(editDigit > 2)
                {
                    // All digits are updated and leave the edit mode.
                    if(time < MINIMUM_TIME)
                    {
                        // Specified time is less than minimum time interval. Reset time to minimum.
                        time = MINIMUM_TIME;
                    }

                    editDigit = 0;
                    targetTime = time;

                    // Load new time value into EEPROM.
                    wdt_reset();
                    if(targetTime != eeprom_read_word((uint16_t * )0))
                    {
                        eeprom_write_word((uint16_t*)0, targetTime);
                    }

                    wdt_reset();
                    currentState = IdleState;
                }
            }
        }

        if(currentState == RunState)
        {            
            if(time <= 0)
            {            
                // UV exposure countdown completed.
                currentState = IdleState;
                time = targetTime;
                PORTA = 0x00;

                // Trigger buzzer with long beeps.
                timerCounter = 1;
                buzzerCounter = 6;
                PORTB = PORTB | 0x80;
            }

            if((PIND & 0x08) == 0x08)
            {
                // Lid has been open.
                PORTA = 0x00;
                currentState = PauseState;                
            }
        }

        if((currentState == PauseState) && ((PIND & 0x08) == 0x00))
        {
            // Lid has been closed.            
            currentState = RunState;
            PORTA = 0x03;
        }

        lastSwitchState = PIND; 
        wdt_reset();
        setNumber();        
    }
        
    return 0;
}

uint8_t extractDigit()
{    
    uint8_t digit1, digit2, digit3;

    // Extract digit values.
    digit1 = time / 100;
    digit2 = (time - (digit1 * 100)) / 10;
    digit3 = time - (int16_t)((digit1 * 100) + (digit2 * 10));

    switch (editDigit)
    {
    case 0:
        return digit1;
    case 1:
        return digit2;
    case 2:
        return digit3;
    }

    return 0;
}

void updateDigit(uint8_t newValue)
{
    uint8_t digit1, digit2, digit3;
    digit1 = time / 100;
    digit2 = (time - (digit1 * 100)) / 10;
    digit3 = time - (int16_t)((digit1 * 100) + (digit2 * 10));

    switch (editDigit)
    {
    case 0:
        digit1 = newValue;
        break;
    case 1:
        digit2 = newValue;
        break;
    case 2:
        digit3 = newValue;
        break;
    }

    // Create new time value with specified digit data and extracted data.
    time = digit3 + (digit2 * 10) + ((int16_t)(digit1 * 100));
}

uint8_t getDigit(uint8_t value, uint8_t showZero)
{
    switch (value)
    {
    case 0:  
        if(currentState == EditState)  
        {
            // In edit mode show all zero digits.
            return 0x3F;
        }
        else
        {
            // Flag to shutdown leading zeros.
            return showZero ? 0x3F : 0x00;
        }                
    case 1:
        return 0x30;
    case 2:
        return 0x5B;
    case 3:
        return 0x4F;
    case 4:
        return 0x66;
    case 5:
        return 0x6D;
    case 6:
        return 0x7D;
    case 7:
        return 0x07;
    case 8:
        return 0x7F;
    case 9:
        return 0x6F;  
    default:
        return 0;
    }
}

uint8_t getDigitMode(uint8_t pos)
{
    if(currentState == EditState)
    {
        if((timerCounter == 1) && (pos == editDigit))
        {
            // Enter into single digit blink mode.
            return 0;
        }
    }
    else if(currentState == PauseState)
    {
        if(timerCounter == 1)
        {
            // Enter into full (3-digit) blink mode.
            return 0;
        }
    }
    
    return (1 << (pos + 3));   
}

void setNumber()
{
    uint8_t digit1, digit2, digit3;
    digit1 = time / 100;
    digit2 = (time - (digit1 * 100)) / 10;
    digit3 = time - (int16_t)((digit1 * 100) + (digit2 * 10));

    // Update 1st digit of the seven segment display.
    PORTB = PORTB & 0xC7;
    setDigitValue(getDigit(digit1, (time > 99)));
    PORTB = PORTB | getDigitMode(0);
    _delay_ms(2);

    // Update 2nd digit of the seven segment display.
    PORTB = PORTB & 0xC7;
    setDigitValue(getDigit(digit2, (time > 9)));
    PORTB = PORTB | getDigitMode(1);
    _delay_ms(2);

    // Update 3rd digit of the seven segment display.
    PORTB = PORTB & 0xC7;
    setDigitValue(getDigit(digit3, 1));
    PORTB = PORTB | getDigitMode(2);
    _delay_ms(2);

    PORTB = PORTB & 0xC7;
}

void setDigitValue(uint8_t value)
{
    // Initialize shift register input terminals to known state.
    int8_t dataPos;
    PORTB = PORTB & 0xF8;

    for(dataPos = 7; dataPos >= 0; dataPos--)
    {
        // Set data value.
        PORTB = PORTB | ((value & (1 << dataPos)) ? 0x01 : 0x00);
        _delay_us(2);

        // Load data into register.
        PORTB = PORTB | 0x02;
        _delay_us(5);
        PORTB = PORTB & 0xF8;
        _delay_us(2);
    }

    // Latch data into output.
    PORTB = PORTB | 0x04;
    _delay_us(5);
    PORTB = PORTB & 0xF8;
}

ISR(TIMER1_COMPA_vect) 
{
    timerCounter++;

    if(timerCounter == 2)
    {
        // 1 second time trigger.
        timerCounter = 0;

        if((currentState == RunState) && (time > 0))
        {
            // Update UV light timer.
            time--;
        }
    }

    // Check status of the buzzer flag.
    if(buzzerCounter > 0)
    {
        buzzerCounter--;

        if(buzzerCounter == 0)
        {
            // Mute buzzer.
            PORTB = PORTB & 0x7F;
        }
        else
        {
            // Activate and mute buzzer terminal based on timer counter value.
            PORTB = (timerCounter % 2) ? (PORTB | 0x80) : (PORTB & 0x7F);
        }                
    }
}