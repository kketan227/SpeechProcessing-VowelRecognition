/* VowelRecognition.cpp : Defines the entry point for the console application.
 Author : Ketan Karnakota
 Roll No.: 204101030
 Vowel Recognition using Cepstral Coefficients.

 VowelRecognition
*/

#include "stdafx.h"
#include <cstdio>
#include <iostream> 
/* fstream header file for ifstream, ofstream,  
   fstream classes */
#include <fstream> 
#include <string>
#include <sstream> // for stringstream

#include "config.h" // Moved the global variables to header file

using namespace std;

long double samples[320000], MaxAmp=0;
long double ThresholdZCR,DCshift=0;
double TotalEnergy=0, ThresholdEnergy, NoiseEnergy=0, TotalZCR=0; //TotalZCR is only for first few InitFrames
long start=0, stop=0; //start and end marker for speech signal maybe used when live implementation done
long words[50]; // To do 25 words. 50/2 for start and stop pairs.
int sampleCount=0, framecount=0, q=p;
int stabframe=0, N=framesize;

double tok_wt[]={1.0, 3.0, 7.0, 13.0, 19.0, 22.0, 25.0, 33.0, 42.0, 50.0, 56.0, 61.0}; // Given by sir/facebook post

bool verbose=false; // For detailed statements or debugging couts

long double* R = new long double[p+1];
string line;
string filename_start="Recordings/204101030_"; // input file name as used by recording module
std::ostringstream oss;


long double* AutoCorrelation (int frameNo){
	long double* x = new long double[N];
	for(int t=0; t<N; t++)
		x[t]=samples[t+(frameNo*N)];
	for (int ind=0; ind<=p; ind++){
		double square = 0;
		for (int t=0; t<N-ind; t++)
			square+= x[t]*x[t+ind];
		R[ind]=square;
	}
	return R;
} //This calculates for FrameNo*N + 320 frames.

long double* Durbin(long double* Ri){
	long double* E = new long double [p+1];
	long double* k = new long double [p+1];
	long double* alpha = new long double [p+1];
	long double* old_alpha = new long double [p+1];

	for(int i =0;i<=p;i++){
		if(verbose)cout<<"Ri"<<i<<"= "<<Ri[i]<<endl;
	}
	if(Ri[0]==0)
	{
		cout << "\nEnergy should not be ZERO\n";
		
	}
	for(int i =0; i<=p;i++){
		alpha[i]=0;
		old_alpha[i]=0;
		E[i]=0;
	} // Init to zero to be safe and not have garbage values

	E[0]=Ri[0];
	
	for(int i=1;i<=p;i++){
		if(i==1) {k[1]=Ri[1]/Ri[0];}
		else{
			long double sum=0;
			for (int j=1;j<=i-1;j++){
				old_alpha[j]=alpha[j];
				//cout<<"old alpha at j "<<j<<" is "<<old_alpha[j]<<endl;
				sum+= old_alpha[j]*Ri[i-j];
			}
			//cout<<"Sum value at "<<i<<"th is "<<sum<<endl;
			k[i]=(Ri[i]-sum)/E[i-1];
		}
		//cout<<"k"<<i<<"= "<<k[i]<<endl;
	
		alpha[i]=k[i];
		for(int j=1;j<=i-1;j++){
			alpha[j]=old_alpha[j]-k[i]*old_alpha[i-j];
			//cout<<"alpha here is "<<alpha[j]<<endl;
		}
		E[i]=(1-k[i]*k[i])*E[i-1];
		//cout<<"E"<<i<<"= "<<E[i]<<endl;
	}
	for(int i =0;i<=p;i++){
		if(verbose)cout<<"Alpha"<<i<<"= "<<alpha[i]<<endl;
	}
	return alpha;
}

long double* Cepstral( long double* alpha){
	long double* new_alpha= new long double [p+1];
	long double* c= new long double [p+1];
	new_alpha = alpha;
	if(verbose)
		cout<<"new"<<endl;
	
	for (int z =0; z<=p; z++){
		//new_alpha[z]*=-1; // to invert alpha sign
	}
	for (int m=1; m<=p;m++){
		long double sum_cep =0;
		for(int k_cep=1; k_cep<=m-1;k_cep++)
			sum_cep+=((double) k_cep/m)*c[k_cep]*new_alpha[m-k_cep];
		c[m]=new_alpha[m]+sum_cep;
		if(verbose)
			cout<<"Cepstral value is "<<c[m]<<endl;
	}
	return c; // This is the array of cepstral coeffecients from 1 to 12 for the same frame from AutoCorrelation
}

double Tokhura( long double* ref, long double* test){
	double sum=0;
	for (int i =1; i<=q;i++){
		sum+= tok_wt[i]*(test[i]-ref[i])*(test[i]-ref[i]);
	}
	return sum;
}

long double* raisedSineWindow (long double* C){
	long double* Cr= new long double [p+1];
	for(int m=1; m<=q;m++){
		//if(verbose) cout<<m<<" th Original Cepstral value "<<C[m];
		Cr[m]=C[m]*(double)(1+(double)q/2*sin((2 * asin(1.0))*m/q));
		//if (verbose) cout<<" and raise value "<<Cr[m]<<endl;
	}
	return Cr;
}

long double* averageOverUtterances (long double* C[NumVowel][NumUtterance/2][NumFrames], int vowelIndex, int FrameIndex){
	long double* avged= new long double [p+1];
	avged[0]=0; // we arent considering c0
	for(int i=1;i<=12;i++){
		long double sum=0;
		for(int b=0;b<10;b++)
			sum+=C[vowelIndex][b][FrameIndex][i]; //every utterance wala loop
		sum/=10;
		//cout<<"avg is "<<sum<<endl;
		avged[i]=sum;
	}// I have for one vowel ka particular frame
	return avged;
}

std::string toString(int i){
   std::stringstream ss;
   ss << i;
   return ss.str();
}

int _tmain(int argc, _TCHAR* argv[])
{
	long double* cep[NumVowel][NumUtterance/2][NumFrames]; // by 2 because half for training and half for testing
	long double* cep_avg[NumVowel][NumFrames]; // Holds the per frame average over the various utterances
	long double* cepTest[NumVowel][NumUtterance/2][NumFrames]; // by 2 because half for training and half for testing
	long double* cepTestLive[NumFrames]; // We are only considering 1 utterance of a vowel and 5 stable frames
	string vowels [] = {"a","e","i","o","u"}; // Used to for file numbering and recognition naming
	cout<<fixed;

	cout<<"Enter 1 if you want to run the training.\nEnter 2 if you want to read the reference file directly."<<
		" (This reads the reference file which is generated by training. Please make sure the training was run atleast once or the reference file has properly precomputed values) "<<endl;
	char choice1;
	cin>>choice1;
	if(choice1=='1'){ 

		// REFERENCE FILE GENERATION OR TRAINING
		ofstream cepout;
		ifstream fin;
		cepout.open("reference.txt");
		cepout<<fixed;
		cout<<"Started to read training files..."<<endl;
		for(int a=0; a<5; a++){ // a is for vowel index
			for(int b=0; b<10; b++){ // b is for utterance index
				string filename= filename_start+vowels[a]+"_"+toString(b+1)+".txt";
				//cout<<filename<<endl;
				DCshift=0;
				int samplecount=0;
				stabframe=0;
				MaxAmp=0;
				fin.open(filename);
				if (!fin){
					cout << "\n**File failed to open**\n"<<filename<<"\n";
					fin.clear();
					system("pause");
					exit(2);	
				}
				if (fin.is_open()){
					while ( !fin.eof() ){ 
						getline (fin,line);
						samplecount+=1;     
						if(samplecount > 0) 
						{
							try{
								samples[samplecount] = stold(line.c_str()); //5=4+1 4-->indicates encoding, 1-->array index starts with 0
							}
							catch (...){}
							if(samplecount < InitFrames*framesize){
								//cout<<samples[samplecount - (IgnoreSamples + 5)]<<endl;
								DCshift+=samples[samplecount]; // DC shift calculated for before the speech starts
							}
							if(abs(samples[samplecount])>MaxAmp){
								MaxAmp=abs(samples[samplecount]);
								stabframe=samplecount;
							}
						}
					}
					fin.close();
				}
				samplecount = samplecount;
				framecount = samplecount/framesize;
				DCshift=DCshift/(InitFrames*framesize);
				if(verbose){
					cout << "Number of Samples = " << samplecount << endl;
					cout << "Number of frames = " << framecount << endl;
					cout << "Maximum Amplitude = " << MaxAmp << endl;
					cout << "DC shift needed is " << DCshift << endl<<endl;
				}
				

				for(int i=0;i<samplecount;i++){
					samples[i]= (samples[i] - DCshift)*Ampscale/MaxAmp;  // updating the samples using normalization
				}
				
				//cout<<samples[stabframe]<<" is this 10000?"<<endl;
				//cout<<samples[stabframe-1]<< "or is this 1000?"<<endl;
			
				stabframe/=320; // Here we are changing to frame number than the sample number

				for (int c =0;c<5;c++){
					R=AutoCorrelation(c+(stabframe-2));
					long double* A=Durbin(R);
					long double* cep_temp=Cepstral(A);
					//get the cepstral values right here after this we update it with raised sine window
					cep[a][b][c] = (cep_temp);
					//cep[a][b][c] = raisedSineWindow(cep_temp); //At cep[vowel][utterance][frame] we have the raised sine windowed cepstral 12 coeffs
				}
			}
		}
		cout<<"Stage 1"<<endl;
		// At this stage we will have the Cis for every vowel, utterance and frame number
		// Now we need to average over the utterances.
		for(int a =0;a<5;a++){
			//cout<<" Hello a "<<a<<endl;
			for(int c=0;c<5;c++){
				//cout<<"Hello c "<<c<<endl;
				/*for(int i=1;i<=12;i++){
					double long sum=0;
					for(int b=0;b<10;b++)
						sum+=cep[a][b][c][i];
					sum/=10;
					cep_avg[a][c][i]=sum;
				}*/
				long double* avgceps = averageOverUtterances(cep,a,c);
				//cout<<"avgceps vector"<<avgceps[1]<<endl;
				cep_avg[a][c]=avgceps;
			}
		} 
		cout<<"Stage 2"<<endl;
		// Now cep_avg has avg cepstral coeffs for every vowel's 5 frames.
		//Now we need to store them into a file for later use.

		for(int a=0;a<5;a++){
			for(int c=0;c<5;c++){
				for(int i=0;i<=12;i++){
					if(i==0) cepout<<"0"<<endl; //I am adding this myself as a delimiter and this isnt actuall the c0 value. We aren't using c0 so I am doing this
					else cepout<<cep_avg[a][c][i]<<endl;
				}
			}
		}
		//cout<<"CEP AVG KA VALUE "<<cep_avg[0][0][1]<<endl;
		cepout.close();
		// At this point we have saved/generated the reference.txt
		cout<<"Training Done"<<endl;
		// REFERENCE FILE GENERATION OR TRAINING IS DONE!!!
	}

	else{
		ifstream cepRead;
		cepRead.open("reference.txt");
		if (!cepRead){
			cout << "\n**File failed to open**\n Please make sure you run the training atleast once and check if reference.txt file is generated."<<endl;
			cepRead.clear();
			system("pause");
			exit(2);	
		}
		if (cepRead.is_open()){
			//while ( !cepRead.eof() ){ 
				long double value;
				for(int a=0;a<5;a++){
					for(int c=0;c<5;c++){
						long double* computedCeps = new long double[p+1];
						for(int i=0;i<=12;i++){
							getline (cepRead,line);     
							try{
								value= stold(line.c_str());
							}
							catch (...){}

							computedCeps[i]=value;

						}
						cep_avg[a][c]=computedCeps;
					}
				}
			//}
		}

		//ofstream out;
		//out.open("Check.txt");
		//out<<fixed;
		//for(int a=0;a<5;a++){
		//	for(int c=0;c<5;c++){
		//		for(int i=0;i<=12;i++){
		//			if(i==0) out<<"0"<<endl; //I am adding this myself as a delimiter and this isnt actuall the c0 value. We aren't using c0 so I am doing this
		//			else out<<cep_avg[a][c][i]<<endl;
		//		}
		//	}
		//}
	
		cepRead.close();
		cout<<"Reading the reference file is done" <<endl;
		// READING THE REFERENCE FILE TO SKIP THE COMPUTATION TIME
	}

	// TESTING PART USING REFERENCE FILE
	cout<<"---------------------------------------------------------------------------------------------------------------------"<<endl;
	cout<<"Enter 1 if you want to use the prerecorded files.\nEnter 2 if you want to try using the live recording."<<
		" (This may not work with complete accuracy because the training was using different mic equipment)"<<endl;
	char choice;
	cin>>choice;
	if(choice=='1'){ // Using Pre Recorded Files
		cout<<"Reading Testing files"<<endl;
		for(int a = 0; a<5;a++){ // for every vowel
			for(int i=11;i<=20;i++){ // to maintain the file number sequences. // for every utterance ie test sample
				ifstream fin;
				string filename= filename_start+vowels[a]+"_"+toString(i)+".txt";
				DCshift=0;
				int samplecount=0;
				stabframe=0;
				MaxAmp=0;
				fin.open(filename);
				if (!fin){
					cout << "\n**File failed to open**\n\n";
					fin.clear();
					system("pause");
					exit(2);	
				}
				if (fin.is_open()){
					while ( !fin.eof() ){ 
						getline (fin,line);
						samplecount+=1;     
						if(samplecount > 0) 
						{
							try{
								samples[samplecount] = stold(line.c_str()); //5=4+1 4-->indicates encoding, 1-->array index starts with 0
							}
							catch (...){}
							if(samplecount < InitFrames*framesize){
								//cout<<samples[samplecount - (IgnoreSamples + 5)]<<endl;
								DCshift+=samples[samplecount]; // DC shift calculated for before the speech starts
							}
							if(abs(samples[samplecount])>MaxAmp){
								MaxAmp=abs(samples[samplecount]);
								stabframe=samplecount;
							}
						}
					}
					fin.close();
				}
				samplecount = samplecount - (IgnoreSamples + 4);
				framecount = samplecount/framesize;
				DCshift=DCshift/(InitFrames*framesize);
				if(verbose){
					cout << "Number of Samples = " << samplecount << endl;
					cout << "Number of frames = " << framecount << endl;
					cout << "Maximum Amplitude = " << MaxAmp << endl;
					cout << "DC shift needed is " << DCshift << endl<<endl;
				}
				
				/*cout<<stabframe<<" is the stab sample for file "<<filename<<"having the value of "<<samples[stabframe]<<endl;
				cout << "Maximum Amplitude = " << MaxAmp << endl;
					cout << "DC shift needed is " << DCshift << endl<<endl;*/

				for(int what=0;what<samplecount;what++)
					samples[what]= (samples[what] - DCshift)*Ampscale/MaxAmp;  // updating the samples using normalization
				
				//cout<<samples[stabframe]<<" is this 10000?"<<endl;
			

				stabframe/=320;// Here we are changing to frame number than the sample number


				for (int c =0;c<5;c++){ // for every stable frame
					R=AutoCorrelation(c+(stabframe-2));
					long double* A=Durbin(R);
					long double* cep_temp=Cepstral(A);
					//get the cepstral values right here after this we update it with raised sine window
					cepTest[a][i-11][c] = cep_temp;
					//cepTest[a][i-11][c] = raisedSineWindow(cep_temp); //At cep[vowel][utterance][frame] we have the raised sine windowed cepstral 12 coeffs
				}
			}
		}
		// At this point we have the Cis for the testing samples and all the computation on the ci values is done. 
		// Now we have to compute the distances.
		for(int a=0;a<5;a++){
			cout<<endl<<" NEXT SET OF TEST FILES "<< endl;
			for(int i=0;i<10;i++){
				long double min_d, sum=0;
				int pred_vowel;
				for(int frame=0; frame<5; frame++)
					sum+=Tokhura(cep_avg[0][frame],cepTest[a][i][frame]);
				min_d=sum; pred_vowel=0; sum=0;
				for(int ref=1; ref<5; ref++){ // To compare with other reference vowels
					sum=0;
					for( int frame=0;frame<5;frame++)
						sum+=Tokhura(cep_avg[ref][frame],cepTest[a][i][frame]);
					if(min_d > sum){
						//cout<<"min dist was "<<min_d<<" with "<<vowels[ref]<<endl;
						min_d=sum;
						pred_vowel=ref; // To know which vowel has min distance
					}
				}
				cout<<"For file "<<vowels[a]<<(i+11)<<" matches with min dist of "<<min_d<<" with \""<<vowels[pred_vowel]<<"\""<<endl;
			}	
		}
	}

	else{
		cout<<"IMP: The next "<<duration<<" number of second after pressing a key to continue will be recorded (This can be changed in \"config.h\").\nPlease speak a vowel continously after a gap of 0.1 seconds.\n"<<
			"This may not have complete accuracy because the training samples was done on another microphone setup.\nAfter the recording please press a key to continue execution"<<endl;
		system("pause");
		oss << recmod << " "<<duration <<" "<<inpw << " " << inpt; // Creation of string for system call
		system(oss.str().c_str()); //call the system function to read directly from the default mic

		cout<<"Recording done. Opening the recorded file."<<endl;
		ifstream fin; 
		string filename= inpt;
		DCshift=0;
		int samplecount=0;
		stabframe=0;
		MaxAmp=0;
		fin.open(filename);
		if (!fin){
			cout << "\n**File failed to open**\n\n";
			fin.clear();
			system("pause");
			exit(2);	
		}
		if (fin.is_open()){
			while ( !fin.eof() ){ 
				getline (fin,line);
				samplecount+=1;     
				if(samplecount > 0) 
				{
					try{
						samples[samplecount] = stold(line.c_str()); //5=4+1 4-->indicates encoding, 1-->array index starts with 0
					}
					catch (...){}
					if(samplecount < InitFrames*framesize){
						//cout<<samples[samplecount - (IgnoreSamples + 5)]<<endl;
						DCshift+=samples[samplecount]; // DC shift calculated for before the speech starts
					}
					if(abs(samples[samplecount])>MaxAmp){
						MaxAmp=abs(samples[samplecount]);
						stabframe=samplecount;
					}
				}
			}
			fin.close();
		}
		samplecount = samplecount - (IgnoreSamples + 4);
		framecount = samplecount/framesize;
		DCshift=DCshift/(InitFrames*framesize);
		if(verbose){
			cout << "Number of Samples = " << samplecount << endl;
			cout << "Number of frames = " << framecount << endl;
			cout << "Maximum Amplitude = " << MaxAmp << endl;
		}
		cout << "DC shift needed is " << DCshift << endl<<endl;

		/*cout<<stabframe<<" is the stab sample for file "<<filename<<"having the value of "<<samples[stabframe]<<endl;
		cout << "Maximum Amplitude = " << MaxAmp << endl;
		cout << "DC shift needed is " << DCshift << endl<<endl;*/

		for(int what=0;what<samplecount;what++)
			samples[what]= (samples[what] - DCshift)*Ampscale/MaxAmp;  // updating the samples using normalization
		
		stabframe/=320;// Here we are changing to frame number than the sample number


		for (int c =0;c<5;c++){ // for every stable frame
			R=AutoCorrelation(c+(stabframe-2));
			long double* A=Durbin(R);
			long double* cep_temp=Cepstral(A);
			//get the cepstral values right here after this we update it with raised sine window
			cepTestLive[c] = cep_temp;
			//cepTestLive[c] = raisedSineWindow(cep_temp); //At cep[vowel][utterance][frame] we have the raised sine windowed cepstral 12 coeffs
		}

		long double min_d, sum=0;
		int pred_vowel;
		for(int frame=0; frame<5; frame++)
			sum+=Tokhura(cep_avg[0][frame],cepTestLive[frame]);
		min_d=sum; pred_vowel=0; sum=0;
		for(int ref=1; ref<5; ref++){ // To compare with other reference vowels
			sum=0;
			for( int frame=0;frame<5;frame++)
				sum+=Tokhura(cep_avg[ref][frame],cepTestLive[frame]);
			if(min_d > sum){
				//cout<<"min dist was "<<min_d<<" with "<<vowels[ref]<<endl;
				min_d=sum;
				pred_vowel=ref; // To know which vowel has min distance
			}
		}
		
		cout<<"For your recording... the matching vowel is \" "<<vowels[pred_vowel]<< " \" with min dist of "<<min_d<<endl;
	}	
		

	system("pause");

	return 0;
}

