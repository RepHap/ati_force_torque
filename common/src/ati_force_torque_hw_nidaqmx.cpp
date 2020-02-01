/****************************************************************
 *
 * Copyright 2019 McGill University and Haply
 *
 * Author: Christian Frisson
 *
 * Date of creation: October 2019
 *
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License LGPL as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License LGPL for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License LGPL along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************/

#include <ati_force_torque/ati_force_torque_hw_nidaqmx.h>

using namespace ATICombinedDAQFT;

ATIForceTorqueSensorHWNIDAQmx::ATIForceTorqueSensorHWNIDAQmx()
 : hardware_interface::ForceTorqueSensorHW(),
 m_daqft(0)
{
    m_daqft = new FTSystem();
}

bool ATIForceTorqueSensorHWNIDAQmx::init() 
{ 
    std::cout << "ATIForceTorqueSensorHWNIDAQmx::init" << std::endl; 

    String * calFile = new String("FT7339.cal");
    int loadCalibrationFileError = m_daqft->LoadCalibrationFile(calFile,1);
    if(loadCalibrationFileError != 0){
        std::cerr << "ATIForceTorqueSensorHWNIDAQmx::init: error loading calibration file ";
        std::cerr << m_daqft->GetErrorInfo();
        std::cerr << std::endl; 
        return false;
    }

    String * torqueUnits = new String("N-m");
    int setTorqueUnitsError = m_daqft->SetTorqueUnits(torqueUnits);
    if(setTorqueUnitsError != 0){
        std::cerr << "ATIForceTorqueSensorHWNIDAQmx::init: error setting torque units to " << torqueUnits;
        std::cerr << m_daqft->GetErrorInfo();
        std::cerr << std::endl; 
        return false;
    }

    int biasCurrentLoadError = m_daqft->BiasCurrentLoad();
    if(biasCurrentLoadError != 0){
        std::cerr << "ATIForceTorqueSensorHWNIDAQmx::init: error biasing out the current load";
        std::cerr << m_daqft->GetErrorInfo();
        std::cerr << std::endl; 
        return false;
    }

    String * deviceName = new String("Dev1");
    float64 sampleFrequency = 1000.0;
    int averaging = 1;
	int firstChannel = 0;
    bool useTempComp = false;
    int startSingleSampleAcquisitionError = m_daqft->StartSingleSampleAcquisition(deviceName,1000.0, 1, 0, false);
    if(startSingleSampleAcquisitionError != 0){
        std::cerr << "ATIForceTorqueSensorHWNIDAQmx::init: error starting single sample acquisition";
        std::cerr << " with device " << deviceName;
        std::cerr << " sample frequency " << sampleFrequency;
        std::cerr << " averaging of " << averaging;
        std::cerr << " first channel " << firstChannel;
        std::cerr << " using temperature compensation " << useTempComp;
        std::cerr << m_daqft->GetErrorInfo();
        std::cerr << std::endl;  
        return false;
    }

    return true; 
}

bool ATIForceTorqueSensorHWNIDAQmx::readFTData(int statusCode, double& Fx, double& Fy, double& Fz, double& Tx, double& Ty, double& Tz)
{ 
    double readings[6];
    int readSingleFTRecordError = m_daqft->ReadSingleFTRecord(readings);
    if(readSingleFTRecordError != 0){
        std::cerr << "ATIForceTorqueSensorHWNIDAQmx::readFTData: error reading F/T records";
        std::cerr << m_daqft->GetErrorInfo();
        std::cerr << std::endl;  
        return false;
    }
    Fx = readings[0];
	Fy = readings[1];
    Fz = readings[2];
    Tx = readings[3];
    Ty = readings[4];
    Tz = readings[5];
    return true; 
}

bool ATIForceTorqueSensorHWNIDAQmx::Reset()
{
    int stopAcquisitionError = m_daqft->StopAcquisition();
    if(stopAcquisitionError != 0){
        std::cerr << "ATIForceTorqueSensorHWNIDAQmx::Reset: error stopping acquisition";
        std::cerr << m_daqft->GetErrorInfo();
        std::cerr << std::endl;  
        return false;
    }
    return true;
}
