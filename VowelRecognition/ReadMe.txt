========================================================================
    CONSOLE APPLICATION : VowelRecognition Project Overview
========================================================================

All the recordings in the prescribed order are saved in Recordings sub directory. The first 10 files of each vowel are for training and the others for testing.
The reference file for the cepstral values after averaging are found in the Reference.txt all the 25 Ci values are in the same file.
It is in the order of 5 frames of first vowel, 5 frames of next and so on. The value 0 is used a delimiter. because we are not using C0 in this assignment, 
this 0 is being stored in c0. 

To run the assignment simply build and run it (with/without debugging). 

It will first ask you in a choice option based entry if you want to retrain the reference file or pressing 2 we can use the already generated file and skip
the training time. This will read the reference file so if that file doesnt exist or if the training is not executed even once it'll fail exiting with code 2.

Once this part is done it'll present another set of choice if you want to use the prerecorded samples txt file which was recorded by me. 50 samples so better to test 
accuracy using this because this has the data from the same mic equipment as the training files.
The other option 2 uses the recording module given to us to record a short sample of 2 seconds that will be used to try to predict the uttered vowel. This may not 
give complete accuracy because of the change of mic equipment but seems to work fine on 2-3 devices that I personally tested.

After this in the end it outputs the predicted vowel and the Tokhura's distance to that vowel from the reference file which was the least.


