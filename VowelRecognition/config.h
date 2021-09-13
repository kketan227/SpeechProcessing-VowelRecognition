#define framesize 320
//#define MaxFrameCount 400
#define SpeechLength 128000
#define IgnoreSamples 0
#define samplingrate 16000
#define InitFrames 1
#define p 12

#define Ampscale 10000 //For normalization decided in class

#define NumVowel 5
#define NumUtterance 20
#define NumFrames 5



// Filenames for the recording module
#define inpw "vowel.wav" //store the signal in wave form in the file mentioned here
#define inpt "vowel.txt" //store the signal in text form in the file mentioned here
#define duration 2 //time given for recording
#define recmod "Recording_Module.exe" //path of recording module provided