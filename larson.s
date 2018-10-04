/*
 * LARSON SCANNER
 * BLAKE JONES-CS107E
 *
 * This code will set up 8 LEDs to light up in order back and forth
 * like a Larson Scanner. This can also be used for 4 LEDs by changing
 * the NUMLED constant to 0x4.
 */

.equ DELAY, 0x3F0000
.equ NUMLED, 0x8

// configure GPIO PINS 20-27 for output
ldr r0, FSEL2
mov r1, #0b1000000000000000000000
orr r1, #0b1000000000000000000
orr r1, #0b1000000000000000
orr r1, #0b1000000000000
orr r1, #0b1000000000
orr r1, #0b1000000
orr r1, #0b1001
str r1, [r0] 

// set bit 20
mov r1, #(1<<20)

//set a counter for the number of LEDs
mov r2, #NUMLED

loop: 

// set GPIO high (Depending on which LED should be turned on) 
ldr r0, SET0
str r1, [r0] 

// delay
mov r4, #DELAY
wait1:
    subs r4, #1
    bne wait1

// set GPIO low
ldr r0, CLR0
str r1, [r0] 

// delay
mov r4, #DELAY
wait2:
    subs r4, #1
    bne wait2

//remove one from LED counter (How many LEDs left to turn on going FORWARD aka 20->27)
//will branch to forward until needs to run in reverse
subs r2, #1
bne forward

//REVERSE: change the GPIO to be set to the next one down (aka right shift)
lsr r1, r1, #1
mov r2, #1

//Reverse LED counter subtracts by one and branches until at 0 (when needs to go forward)
subs r3, #1
bne loop

//Reset the forward counter and loop
mov r2, #NUMLED
b loop

//FORWARD: change GPIO to be the next one up (aka left shift)
//Also, sets the reverse LED counter (-1 b/c the last light already lit up)
forward:
    mov r3, #NUMLED
    sub r3, #1
    lsl r1, r1, #1
    b loop

FSEL0: .word 0x20200000
FSEL1: .word 0x20200004
FSEL2: .word 0x20200008
SET0:  .word 0x2020001C
SET1:  .word 0x20200020
CLR0:  .word 0x20200028
CLR1:  .word 0x2020002C

