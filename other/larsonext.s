/*
 * LARSON SCANNER EXTENSION
 * BLAKE JONES-CS107E
 *
 * This code will set up 8 LEDs to light up in order back and forth
 * like a Larson Scanner. This can also be used for 4 LEDs by changing
 * the NUMLED constant to 0x4. This scanner has three different brightness
 * levels. The middle one follows the traditional pattern levels, and the
 * LEDs to the left and right follow the middle LED at a lower brightness.
 * The LEDs two spots away on each side also follow the middle LED at a very
 * low brightness level.
 */

.equ DELAY, 0xf00000
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

// set bit 20, the two bits next to it, and the two bits two away
mov r1, #(1<<20)
mov r5, #(0b111<<19)
mov r6, #(0b11111<<18)
//set a counter for the number of LEDs
mov r2, #NUMLED
//set a timer for determining brightness levels
mov r7, #20

loop: 

// set GPIO high (Depending on which LED should be turned on) 
ldr r0, SET0
str r1, [r0] 

// delay in a sequence to create 3 brightness levels
mov r4, #DELAY
wait1:
    sub r7, #1
    ldr r0, SET0
    str r1, [r0]
    cmp r7, #3 //loop until r7 brightness counter ends at value  (makes middle light very bright)
    bgt wait1
    ldr r0, SET0 //no need to clear since middle LED still being lit up
    str r5, [r0]
    cmp r7, #1 //loop until r7 counter ends at value (makes next two lights medium brightness)
    bgt wait1
    ldr r0, SET0
    str r6, [r0]
    ldr r0, CLR0 //clear the registers to reset all the LEDs
    str r6, [r0]
    mov r7, #20 //reset the brightness counter
    subs r4, #1 //loop until the delay counter reaches 0
    bne wait1

//remove one from LED counter (How many LEDs left to turn on going FORWARD aka 20->27)
//will branch to forward until needs to run in reverse
subs r2, #1
bne forward

//REVERSE: change the GPIO to be set to the next one down (aka right shift)
lsr r1, r1, #1
lsr r5, r5, #1
lsr r6, r6, #1
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
    lsl r5, r5, #1
    lsl r6, r6, #1
    b loop

FSEL0: .word 0x20200000
FSEL1: .word 0x20200004
FSEL2: .word 0x20200008
SET0:  .word 0x2020001C
SET1:  .word 0x20200020
CLR0:  .word 0x20200028
CLR1:  .word 0x2020002C

