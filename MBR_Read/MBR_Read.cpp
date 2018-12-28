// MBR_Read.cpp : Defines the entry point for the console application.
//
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sys/types.h>

#include "stdafx.h"

using namespace std;


struct MBR_Packet {
public:								
	static const int PACKET_SIZE = 1056;

	MBR_Header Header;
	MBR_Data Data;

	void operator<<(ifstream &DataPacket) {
	
		Header << DataPacket;
		Data << DataPacket;
	}

	int fWrite(ofstream &OutFile, const int Mode, const bool Bin_or_ASCII) {					// Mode: 0(or any Number other than 1(or)2) Write full packet, 1  Write only header, 2 Write only Data				// Bin_or_ASCII: 0 Write in Bin, 1 Write in ASCII

		switch (Mode)
		{
		default:
			Header.fWrite(OutFile, Bin_or_ASCII);
			Data.fWrite(OutFile, Bin_or_ASCII);
			break;

		case 1:
			Header.fWrite(OutFile, Bin_or_ASCII);
			break;

		case 3:
			Data.fWrite(OutFile, Bin_or_ASCII);
			break;

		}		
	}
}; 
MBR_Packet MBR_NULL_Packet = { { (unsigned long long)0, {'\0'}, (unsigned int)0}, {(int8_t)0, (int8_t)0} };

struct MBR_Data{
public:

	static const int DATA_SIZE = 1024;

	Doublet<int8_t> E[DATA_SIZE / 2];

	void operator<<(ifstream &DataIn) {

		DataIn.read((char *)&E, DATA_SIZE);
	}

	int fWrite(ofstream &OutFile, const bool Bin_or_ASCII) {					// Bin_or_ASCII: 0 Write in Bin, 1 Write in ASCII)

		if(Bin_or_ASCII == 0)												    OutFile.write((char *)&E, DATA_SIZE);
		else					for (int i = 0; i < DATA_SIZE/2; i++)			OutFile << E[i].x << "," << E[i].y << "\n";
		
		return 0;
	}
};


template<class T>
struct Doublet {
public:

	T x;
	T y;
};


struct MBR_Header {															// Need to properly Update
public:

	static const int HEADER_SIZE = 32; 

	unsigned long long DAS_ID;		// DAS ID
	char Unknown[24];
	unsigned int PacketC;			// Packet count

	void operator<<(ifstream &HeaderIn) {

			HeaderIn.read((char *)&DAS_ID, 8);
			HeaderIn.read((char *)&Unknown, 24);
			HeaderIn.read((char *)&PacketC, 4);
	}

	int fWrite(ofstream &OutFile, const bool Bin_or_ASCII) {					// Bin_or_ASCII: 0 Write in Bin, 1 Write in ASCII)

		if (Bin_or_ASCII == 0)										    OutFile.write((char *)this, HEADER_SIZE);
		else {
		
			OutFile << "MBRDSP(ID):" << DAS_ID << "\n";
			OutFile << "Unknown:" << Unknown[24] << "\n";
			OutFile << "Packet_Count:" << PacketC << "\n";
			OutFile << "------------------------------------------------------------------------------\n";
		}

		return 0;
	}
};


int main(char **argv, int argc){

	string IN_PATH__, OUT_PATH__;
	int DAS_ID;

	if (string(argv[1]) == "-h" || string(argv[1]) == "--help") {
		cout << "Useage: MBR_Read  <input file PATH>  <output file PATH> DAS_ID";
		exit(0);
	}

	if (argc < 4) {
		cout << "Too few arguements.................\n ";
		cout << "Useage: MBR_Read  <input file PATH>  <output file PATH> DAS_ID";
		exit(-1);
	}
	else if (argc == 4) {
		IN_PATH__ = string(argv[1]);
		OUT_PATH__ = string(argv[2]);

		DAS_ID = strtol(argv[3], NULL, 10);

	}
	else if (argc > 4) {

		cout << "Too many arguements.................\n ";
		cout << "Useage: MBR_Read  <input file PATH>  <output file PATH> DAS_ID";
		exit(1);
	}


	ifstream MBR_DataIn;

	MBR_DataIn.open(IN_PATH__, ios::in | ios::binary);											// Opening Data File

	if (!MBR_DataIn.is_open()) {
		cout << " Error Opening the file, please recheck the Input Filepath and Try Again \n";
		cout << "Format: Spectrogram  <input file PATH>  <output file PATH> Sample_rate(in Hz) Time_Resolution(in ms) \n";

		exit(2);
	}
	MBR_DataIn.seekg(ios::beg);


	string FILE_NAME__ = IN_PATH__.substr(IN_PATH__.find_last_of('/'));

	ifstream OutFileTemp; OutFileTemp.open(OUT_PATH__ + FILE_NAME__ + "_Ex_Ey_" + " .bin", ios::in);

	if (OutFileTemp.is_open()) {
		cout << " Output file already exists, please use a different name and Try Again \n";
		cout << "Useage: MBR_Read  <input file PATH>  <output file PATH> DAS_ID";

		exit(3);
	}
	OutFileTemp.close();

	ofstream MBR_DataOut;
											
	MBR_DataOut.seekp(ios::beg);

	const int MAX_PACKETS = 256*1024;

	MBR_Packet MBR[MAX_PACKETS];

	while (!MBR_DataIn.eof()){
												// In chunks of 265*1024 packets == 264MB RAM Required
		Load_MBR(MBR, MBR_DataIn, DAS_ID, MAX_PACKETS);


	}

    return 0;
}

int Load_MBR(MBR_Packet *MBR, ifstream &MBR_DataIn, int DAS_ID, const int MAX_PACKETS) {

	int i = 0;

	while (i < MAX_PACKETS && !MBR_DataIn.eof())
	{
		MBR[i] << MBR_DataIn;
		if(MBR[i].Header.DAS_ID == DAS_ID)	 i++;
	}
	if (MBR_DataIn.eof() && MBR[i - 1].Header.DAS_ID != DAS_ID)			

		if (MBR_DataIn.eof()) {
			
			
			while (i < MAX_PACKETS) { MBR[i] = MBR_NULL_Packet;		i++; }
		}
	return 0;
}

int fWrite_MBR(ofstream &MBR_DataOut, MBR_Packet *MBR, const int Mode, const bool Bin_or_ASCII, const int MAX_PACKETS) {		// Write them in order

	int i = 0;
	unsigned int prev_Packet_No = MBR[i].Header.PacketC;


	while (i < MAX_PACKETS) {

		if (MBR[i].Header.PacketC < prev_Packet_No) {
			MBR_DataOut.seekp(MBR_Packet::PACKET_SIZE *(MBR[i].Header.PacketC < (prev_Packet_No + 1)), ios::cur);

			MBR[i].fWrite(MBR_DataOut, Mode, Bin_or_ASCII);
		}

		for (int j = 1; j < MBR[i].Header.PacketC - prev_Packet_No; j++)			MBR_NULL_Packet.fWrite(MBR_DataOut, Mode, Bin_or_ASCII);		//

		MBR[i].fWrite(MBR_DataOut, Mode, Bin_or_ASCII);
		prev_Packet_No = MBR[i].Header.PacketC;
	}
	return 0;
}
