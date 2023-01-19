/*
  Copyright 2018, 2021 Medical Image Processing Group
              Department of Radiology
            University of Pennsylvania

This file is part of CAVASS.

CAVASS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

CAVASS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CAVASS.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include  "wx/wx.h"
#include  "wx/filename.h"
#include <Viewnix.h>
#include  "cv3dv.h"
#include <assert.h>
#include  "port_data/from_dicom.h"
#include <stdlib.h>
#include <ctype.h>
#include  "Dicom.h"




int main(int argc, char *argv[])
{
	float specified_weight=0.;
	char *starttime=NULL, *startdate=NULL, *dose=NULL;

	if (argc>2 && sscanf(argv[2], "weight=%f", &specified_weight)==1)
	{
		if (argc>3 && strncmp(argv[3], "starttime=", 10)==0)
		{
			starttime = argv[3]+10;
			if (argc>4 && strncmp(argv[4], "startdate=", 10)==0)
			{
				startdate = argv[4]+10;
				if (argc>5 && strncmp(argv[5], "dose=", 5)==0)
				{
					dose = argv[5]+5;
					argc--;
				}
				argc--;
			}
			argc--;
		}
		argc--;
	}
	if (argc != 2)
	{
		fprintf(stderr, "Usage: bqml <DICOM_file> [weight=<patient_weight> [starttime=<start_time> [startdate=<start_date> [dose=<total_dose>]]]]\n");
		exit(1);
	}

	FILE *fp=fopen(argv[1], "rb");
	if (fp == NULL)
	{
		fprintf(stderr, "Cannot open %s\n", argv[1]);
		exit(1);
	}

    int ris=100, ii, has_PET_info;
    char buf[256];
    float av;
	double final_factor_bqml;

    ris = get_element(fp, 0x0054, 0x0016, AT, buf, 256, &ii);
    if (ris != 100)
    {
        float PatientWeight=specified_weight, SeriesTime,
            RadiopharmaceuticalStartTime,
            RadionuclideTotalDose, RadionuclideHalfLife,
            RadionuclidePositronFraction;
        DicomDictionary dd=DicomDictionary(false);
        DicomReader dr=DicomReader((const char *)
            argv[1], dd);
        DicomDataElement *RST=NULL;
		if (starttime == NULL)
		{
			RST = dr.findEntry(&dr.mRoot, 0x0018, 0x1072);
			if (RST)
				starttime = RST->cData;
        }
		DicomDataElement *RTD=NULL;
		if (dose == NULL)
		{
			RTD = dr.findEntry(&dr.mRoot, 0x0018, 0x1074);
			if (RTD)
				dose = RTD->cData;
        }
		DicomDataElement *RHL=dr.findEntry(&dr.mRoot, 0x0018, 0x1075);
        DicomDataElement *RPF=dr.findEntry(&dr.mRoot, 0x0018, 0x1076);
        if (starttime && dose &&
				RHL && RHL->cData)
		{
			has_PET_info = get_element(fp,
				0x0010, 0x1030, AN, buf, 256, &ii)==0 &&
				(specified_weight>0 ||sscanf(buf, "%f", &PatientWeight)==1) &&
				get_element(fp, 0x0008, 0x0031, AN, buf, 256, &ii)==0 &&
				sscanf(buf, "%f", &SeriesTime)==1 &&
				sscanf(starttime, "%f", &RadiopharmaceuticalStartTime)==1 &&
				sscanf(dose, "%f", &RadionuclideTotalDose)==1 &&
				sscanf(RHL->cData, "%f", &RadionuclideHalfLife)==1 &&
				((RPF && RPF->cData &&
			     sscanf(RPF->cData, "%f",&RadionuclidePositronFraction)==1) ||
			     (RadionuclideHalfLife>6580 && RadionuclideHalfLife<6590 &&
			     (RadionuclidePositronFraction=0.9673)) /* ^{18}F */);
			if (has_PET_info)
			{
				int days=0;
				DicomDataElement *STD=dr.findEntry(&dr.mRoot, 0x0008, 0x0021);
				DicomDataElement *RSD=NULL;
				if (startdate == NULL)
				{
					RSD = dr.findEntry(&dr.mRoot, 0x0018, 0x1078);
					if (RSD)
						startdate = RSD->cData;
				}
				if (STD && STD->cData && startdate)
				{
					wxDateTime start_date, end_date;
					wxString::const_iterator end;
					if (start_date.ParseFormat(startdate,
							"%Y%m%d%H%M%S", &end) &&
							end_date.ParseFormat(STD->cData,
							"%Y%m%d", &end))
					{
						start_date.ResetTime();
						days = (end_date-start_date).GetDays();
					}
				}
				double hours, minutes;
				hours = floor(SeriesTime/10000);
				SeriesTime -= hours*10000;
				minutes = floor(SeriesTime/100);
				SeriesTime -= minutes*100;
				if (minutes>=60 || SeriesTime>=60)
					wxMessageBox("Invalid series time");
				minutes += hours*60;
				SeriesTime += minutes*60;
				hours = 24.*days;
				hours += floor(RadiopharmaceuticalStartTime/10000);
				RadiopharmaceuticalStartTime -= hours*10000;
				minutes = floor(RadiopharmaceuticalStartTime/100);
				RadiopharmaceuticalStartTime -= minutes*100;
				if (minutes>=60 || RadiopharmaceuticalStartTime>=60)
					wxMessageBox( "Invalid radiopharmaceutical start time");
				minutes += hours*60;
				RadiopharmaceuticalStartTime += minutes*60;
				double total_diff_time,
					time_factor, val_exp, activity_corr;
				total_diff_time =
					SeriesTime-RadiopharmaceuticalStartTime;
				time_factor = total_diff_time/RadionuclideHalfLife;
				val_exp = pow(2.0, -time_factor);
				activity_corr = val_exp*RadionuclideTotalDose;
				final_factor_bqml = PatientWeight*1000/activity_corr/
					RadionuclidePositronFraction;
				printf("%.9f\n", final_factor_bqml);
			}
			delete RST;
			delete RTD;
			delete RHL;
			delete RPF;
		}
	}
	exit(0);
}
