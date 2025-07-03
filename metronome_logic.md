You will make some changes on the, some certain edits and changes on the way the digital, on the way our metronome
functions. So, the first thing is when the metronome, when it's powered on, it will default to the to the main
screen. And the main screen will have the BPM, and it will also have the, it will also show the time signature. As well
as the playing, the playing thing, which I think is already implemented now. But also, there will be four sets. We will
include this functionality whereby there will be four sets. You can set four sets, and then they will change, they will
transition while playing. So, first of all, before we talk about that, let's, let's first just brush up on some
things. We're using an ESP32 now, but later we're going to switch to the more like a prototype, which we're going to use
XIAO ESP32-S3. But that comes later. So, currently, our setup has two buttons and a potentiometer, which is a, which is
a BK10. But later, we're planning to add another potentiometer. We're planning to change this BK10 with a rotary
encoder. So, that one is going to be, that's when, that one's going to have the, you can change the bit with the
scrolling, and also it's going to have a push button. But since, but since currently we don't have the push button on
our BK10 potentiometer, we're going to use one of some, some of the buttons just to emulate the push button on the
potentiometer. So, I will explain this later. So, button one is connected to D12 of the ESP32, and button two is
connected to D15.




So here's how it's going to work. The rotary encoder, but now we're using a potentiometer, so keep in mind. The
potentiometer is only used to adjust the BPM of the selected time scale. So this can be either the beat, which I mean
the measure, or the notes. So this is either the numerator or the denominator. So it either increases it or reduces
it. And then the BPM also increases it or reduces it. And then, so when the potentiometer is turned on, we have the main
screen default, which shows the BPM, the time signature, and it's also going to show the sets. Like what set is it right
now while trying to measure the time. Or while playing. You can have more input in this and let me know which is the
better approach, because if you have any knowledge of a digital metronome. So when we're on the main screen, so the
button D12, when it is long pressed, I think we already have that functionality, it's going to enter the settings
mode. But now we will have, it's going to start with set 1, set 2, set 3, set 4. So it will start with set 1. So set 1,
we will now be able to set for BPM. And then when I press the, when I long press to switch to the settings mode. And
then, if I press the button again, D12, this is not long pressing now. While it's in the setting mode, if I press the
button D12, it's gonna just loop between BPM, beat notes, or BPM or the numerator, which is the beat, and then loop
again to the notes, which is the denominator, which is the measure. So it's gonna loop just like that. It shouldn't go
back or it should stay there on set 1. And then when I'm done setting, so I'm just looping, then I'm changing it with
the rotary encoder. When I'm done setting this, I will press the button D15. The button D15, while in setting mode, it's
going to, when pressed, it will save the current set. So set 1 saved. And then it will switch the screen to set 2. And
then we're gonna do the same on set 2, but we will use button D12 to switch between either BPM, beat or notes. And then
we can adjust the size with the potentiometer turn. And then after that, I'll press the D15, it's gonna go to
set 3. Then when I'm done setting all the four sets, I will long press D15. So I long press D15 to start. But as I told
you, we're just using long press now for D15, but this is temporary. This is just for now. Later, we will not need to
long press D15. When we switch to the rotary encoder, we will use the rotary encoder push, rotary push button to now
switch to the main screen.


So essentially, the button D15, that's used to save every set individually while we're on the settings mode. And as I told you before, settings mode is accessed by long pressing button D12. So here's the thing, because we have these four sets and we are using an ESP32, we might require some memory. So as to my knowledge, ESP32 has SRAM and it's got memory capabilities. So we will use that to save the sets. So that's that. Also, why do we need this? Okay, this is because, why do we need this functionality? So this is because the client wants something like a click bit sound effect, or like a click rhythm. So that's why we need these custom set lists. And we need to create these four set lists with multiple tempos and time signatures. Also, another thing is I'm not sure how long one set should play before switching. So perhaps you can guide on this. Another thing you will do is, you will also increase the pitch or the volume on the buzzer, because I've realized it's very low. The volume is quite low. So you will increase the pitch. But for now, we will not have the functionality to increase the pitch in the encoder. No. So for now, it's just the BPM, the time signature. So for now, it's just the BPM, the time signature. Also, another thing is, while it's playing on the main screen, the default screen, like after we've set all the four sets, I'm not sure what, on the main default, when a user turns the potentiometer, I'm not sure if it should increase the BPM. Yeah, that's the thing. Or do you think we should add an extra mode, whereby this is just one? This is not playing four sets. This is just, it only plays the custom, the one set, which you only need BPM times signature. So I'm not sure if we should do that. One set, which you only need BPM times signature. So yeah, this is now the way the code should work now. So you will work on this. We can take one step at a time, just testing one thing at a time. We don't have to edit the whole code at once, but do it as you see best. Also, yeah, I think, also, yeah, the pins are the same. I haven't changed the pin numbers or anything. So we will work on the code as it is right now and implement these changes.

