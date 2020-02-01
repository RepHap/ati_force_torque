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

#ifndef ATIFORCETORQUESENSORHWNIDAQMX_INCLUDEDEF_H
#define ATIFORCETORQUESENSORHWNIDAQMX_INCLUDEDEF_H

#include <iostream>
#include <fstream>
#include <Eigen/Core>

#include "ati_compiler.h"
#include "ATICombinedDAQFT.h"

#include <force_torque_sensor/force_torque_sensor_hw.h>

class ATIForceTorqueSensorHWNIDAQmx : public hardware_interface::ForceTorqueSensorHW
{
public:
  ATIForceTorqueSensorHWNIDAQmx();
  ATIForceTorqueSensorHWNIDAQmx(int type, std::string path, int baudrate, int base_identifier) {}
  virtual ~ATIForceTorqueSensorHWNIDAQmx() {}

  virtual bool init();
  virtual bool initCommunication(int type, std::string path, int baudrate, int base_identifier) { return true; }
  virtual bool readFTData(int statusCode, double& Fx, double& Fy, double& Fz, double& Tx, double& Ty, double& Tz);
  virtual bool readDiagnosticADCVoltages(int index, short int& value) { return true; }

  bool SetBaudRate(int baudrate){return true;}
  bool SetBaseIdentifier(int identifier){return true;}
  bool Reset();

private:
  ATICombinedDAQFT::FTSystem *m_daqft;
};

#endif
