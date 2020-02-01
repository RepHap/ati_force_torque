/*
ATIDAQHardwareInterface.cpp
implementation of ATIDAQHardwareInterface Class

History
Dec.13.2004 - Sam Skuce (ATI Industrial Automation) - Initial Version Started
july.22.2005 - ss - added support for users to set connection mode
dec.10.2007 - SS - Changed calls to ati_itoa to itoa_s, removed ati_itoa().
*/
#include "Stdafx.h"
#include "ati_compiler.h"
#include "ATIDAQHardwareInterface.h"
#include <string.h> 
#include <stdlib.h>

#define MIN_SAMPLES_PER_CHANNEL 2 /*the minimum number of samples per channel that daqmx will allow us to specify*/


ATIDAQHardwareInterface::ATIDAQHardwareInterface()
{
	this->m_thDAQTask = new TaskHandle;
	SetConnectionMode( DAQmx_Val_Diff );
}

ATIDAQHardwareInterface::~ATIDAQHardwareInterface()
{
	/*stop and clear tasks, without regard to error*/
	DAQmxClearTask( *m_thDAQTask );
	/*delete unmanaged pointers*/
	delete m_thDAQTask;
}

String *ATIDAQHardwareInterface::GetErrorInfo()
{
	char errorBuffer[2048]; /*c-string representation of error description*/
	DAQmxGetExtendedErrorInfo( errorBuffer, 2048 );
	m_sErrorInfo = new String( errorBuffer );
	ati_DIAGNOSTICS_DEBUG_WRITELINE( m_sErrorInfo );
	
	return m_sErrorInfo;
}

String *ATIDAQHardwareInterface::GetErrorCodeDescription( long errCode )
{
	char errorBuffer  __nogc[WHOLE_LOTTA_CHARACTERS]; /*c-string style representation of error description*/
	DAQmxGetErrorString( errCode, errorBuffer, WHOLE_LOTTA_CHARACTERS );
	return new String( errorBuffer );
}

int32 ATIDAQHardwareInterface::ReadSingleSample( float64 buffer __gc[] )
{
	int32 retVal; /*the return code*/
	unsigned long numRawSamples; /*the number of raw gauge value samples*/
	numRawSamples = m_uiNumChannels * m_uiAveragingSize;
	unsigned int i, j; /*loop/array indices*/
	float64 * rawBuffer = new float64[numRawSamples]; /*the buffer which holds the
												raw, unaveraged gauge values*/	
	float64 timeOut = ( numRawSamples / m_f64SamplingFrequency ) + 1; /*allow a full second
		for Windows timing inaccuracies	(definitely overkill, but whatever)*/
	int32 read; /*number samples read*/
	retVal = DAQmxReadAnalogF64( *m_thDAQTask, m_uiAveragingSize, timeOut, DAQmx_Val_GroupByScanNumber, 
				rawBuffer, numRawSamples, &read, NULL );

	for ( i = 0; i < m_uiNumChannels; i++ )
	{
		/*sum the raw values, storing the sum in the first raw data point*/
		for ( j = 1; j < m_uiAveragingSize; j++ )
		{
			rawBuffer[i] += rawBuffer[ i + ( j * m_uiNumChannels ) ]; 
		}
		/*store the average values in the output buffer*/
		buffer[i] = rawBuffer[i] / m_uiAveragingSize;
	}	

	delete []rawBuffer; /*gotta keep up with those unmanaged pointers*/
	 
	return retVal;
}

int32 ATIDAQHardwareInterface::StopCollection()
{
	int32 retVal; /* the return value*/
	retVal = DAQmxClearTask( *m_thDAQTask );	
	return retVal;
}


char __gc* ATIDAQHardwareInterface::ati_strncat( char   *dest, char const __gc* catsource, unsigned int len )
{
	unsigned long insertIndex = ati_nstrlen( dest, len ); /*the index we're inserting at*/
	unsigned long catSourceIndex = 0; /*the index in catsource we're currently reading*/
	while ( ( insertIndex < len ) && ( catsource[catSourceIndex] != '\0' ) )
	{
		dest[insertIndex] = catsource[catSourceIndex];
		insertIndex++;
		catSourceIndex++;
	}
	dest[insertIndex] = '\0';
	return dest;
}

char __gc* ATIDAQHardwareInterface::ati_strncpy( char __gc* dest, char const __gc* source, unsigned int len )
{
	unsigned long sourceLen = ati_nstrlen( source, len ); /*length of source data / number of chars
														  to copy*/
	unsigned long i;  /*generic loop/array index*/
	for ( i = 0; i < sourceLen; i++ )
	{
		dest[i] = source[i];
	}
	if ( sourceLen == len ){ /*if source doesn't fit into dest, ensure that we've properly terminated the
		c-string*/
		dest[len] = '\0';
	}
	return dest;
}

unsigned long ATIDAQHardwareInterface::ati_nstrlen( char const __gc* buffer, unsigned long buffersize )
{
	unsigned long curIndex = 0; /*the current index we're examing in buffer*/
	while ( ( curIndex < buffersize ) && ( buffer[curIndex] != '\0' ) )
		curIndex++;
	return curIndex;
}

int32 ATIDAQHardwareInterface::ati_Channel_Name_Format( char * channelString, 
														ati_SYSTEM_NAMESPACE::String __gc *m_sDeviceName )
{
	int i;
	int advFormat = 0;
	char tempString[4]; /*used as a temporary buffer for translating numbers to strings*/

	/* copy managed device name to unmanaged device name.  Made trickier by the fact that 
	 * the managed (String) type uses unicode, and the NI-DAQmx library uses ASCII
	 */

	for ( i = 0; ( i < m_sDeviceName->Length ) && ( i < ( WHOLE_LOTTA_CHARACTERS - 1 ) ); i++ )
	{
		channelString[i] = (char)(m_sDeviceName->get_Chars(i));
		if ( '/' == channelString[i] )		/* check for a slash in the Device Name string */
			advFormat = 1;
	}	
	channelString[m_sDeviceName->Length] = '\0';	
	
	if ( 0 == advFormat ) /* If we never saw a slash, then we add channel info to the name now */
	{
		ati_strncat( channelString, "/ai", WHOLE_LOTTA_CHARACTERS );
		_itoa_s( m_uiFirstChannel, tempString, 4, 10 );
		ati_strncat( channelString, tempString, WHOLE_LOTTA_CHARACTERS );
		ati_strncat( channelString, ":", WHOLE_LOTTA_CHARACTERS );
		_itoa_s( m_uiFirstChannel + m_uiNumChannels - 1, tempString, 4, 10 );
		ati_strncat( channelString, tempString, WHOLE_LOTTA_CHARACTERS );		
	}

	return 0;
}

int32 ATIDAQHardwareInterface::ConfigSingleSampleTask( float64 sampleRate, 
														int averaging, 
														String * deviceName, 
														int firstChannel,
														int numChannels, 
														int minVoltage, 
														int maxVoltage )
{
	int32 retVal; /*the return value*/
	/*we have to use unmanaged c-style strings in here because that's what the NI-DAQmx 
	C Library uses*/
	char channelString[WHOLE_LOTTA_CHARACTERS]; /*the channel string, of format "Dev1/ai0:5"*/	
	/*set up member data*/
	m_f64SamplingFrequency = sampleRate;
	m_uiAveragingSize = ( averaging > 0 )? averaging : 1; /*averaging must be at least 1*/
	m_sDeviceName = ati_SYSTEM_NAMESPACE::String::Copy(deviceName);
	m_uiFirstChannel = firstChannel;
	m_uiNumChannels = numChannels;
	m_iMinVoltage = minVoltage;
	m_iMaxVoltage = maxVoltage;

	unsigned int numSamplesPerChannel = m_uiAveragingSize; /*the number of samples per channel that
														   daqmx is configured with*/
	/*in a perfect world, NI-DAQmx would allow us to set up single scan acquisitions, but they don't.
	even if we were to make the single sample task a finite sample task, they still require you to use
	at least 2 samples per channel.  Therefore, we pass daqmx a number of samples per channel that it
	will accept, and then set up our task to only read the most recent samples*/
	if ( MIN_SAMPLES_PER_CHANNEL > numSamplesPerChannel ) numSamplesPerChannel = MIN_SAMPLES_PER_CHANNEL;

	ati_Channel_Name_Format( channelString, m_sDeviceName );	/* Format the channel name for niDAQmx */

	/*if the following confuses you, I suggest you read the NI-DAQmx C Reference Help, included
	with NI-DAQmx*/	
	StopCollection(); /*stop currently running task*/
	/*if any function fails (returns non-zero), don't execute any more daqmx functions*/
	/*create the daqmx task*/
	if( !( retVal = DAQmxCreateTask( "", m_thDAQTask ) ) )
		/*add the analog input channels to the task - july.22.2005 - ss - now uses m_iConnectionMode*/
		if( !( retVal = DAQmxCreateAIVoltageChan( *m_thDAQTask, channelString, "", m_iConnectionMode, 
					m_iMinVoltage, m_iMaxVoltage, DAQmx_Val_Volts, NULL ) ) )
			/*set up timing for the task*/
			if( !( retVal = DAQmxCfgSampClkTiming( *m_thDAQTask, NULL, m_f64SamplingFrequency, 
						DAQmx_Val_Rising, DAQmx_Val_ContSamps, numSamplesPerChannel ) ) )
				/*set read position relative to next sample to be read*/
				if( !( retVal = DAQmxSetReadRelativeTo( *m_thDAQTask, DAQmx_Val_MostRecentSamp ) ) )
					/*offset of -1 from the next sample, meaning we read the most recent sample*/
					if( !( retVal = DAQmxSetReadOffset( *m_thDAQTask, 0 ) ) )
						/*start the task*/
						retVal = DAQmxStartTask( *m_thDAQTask );						
	return retVal;
}


String * ATIDAQHardwareInterface::GetDeviceName()
{
	return String::Copy( m_sDeviceName );
}

int32 ATIDAQHardwareInterface::ConfigBufferTask( float64 sampleRate, int averaging, String * deviceName, int firstChannel,
		int numChannels, int minVoltage, int maxVoltage, int bufferSize )
{
	int32 retVal; /*the return value*/
	/*we have to use unmanaged c-style strings in here because that's what the NI-DAQmx 
	C Library uses*/
	char channelString[1024]; /*the channel string, of format "Dev1/ai0:5"*/	
	/*set up member data*/
	m_f64SamplingFrequency = sampleRate;
	m_uiAveragingSize = ( averaging > 0 )? averaging : 1; /*averaging must be at least 1*/
	m_sDeviceName = ati_SYSTEM_NAMESPACE::String::Copy(deviceName);
	m_uiFirstChannel = firstChannel;
	m_uiNumChannels = numChannels;
	m_iMinVoltage = minVoltage;
	m_iMaxVoltage = maxVoltage;
	m_ulBufferedSize = bufferSize;
	unsigned int numSamplesPerChannel = m_uiAveragingSize * m_ulBufferedSize; /*the number of samples per channel that
														   daqmx is configured with*/
	/*NI-DAQmx requires a minimum number of samples per channel*/
	if ( MIN_SAMPLES_PER_CHANNEL > numSamplesPerChannel ) numSamplesPerChannel = MIN_SAMPLES_PER_CHANNEL;

	ati_Channel_Name_Format( channelString, m_sDeviceName );	/* Format the channel name for niDAQmx */

	/*if the following confuses you, I suggest you read the NI-DAQmx C Reference Help, included
	with NI-DAQmx*/	
	StopCollection(); /*stop any currently running task*/
	/*if any function fails (returns non-zero), don't execute any more daqmx functions*/
	/*create the daqmx task*/
	if( !( retVal = DAQmxCreateTask( "", m_thDAQTask ) ) )
		/*add the analog input channels to the task - july.22.2005 - ss - now uses m_iConnectionMode*/
		if( !( retVal = DAQmxCreateAIVoltageChan( *m_thDAQTask, channelString, "", m_iConnectionMode, 
					m_iMinVoltage, m_iMaxVoltage, DAQmx_Val_Volts, NULL ) ) )
			/*set up timing for the task*/
			if( !( retVal = DAQmxCfgSampClkTiming( *m_thDAQTask, NULL, m_f64SamplingFrequency, 
						DAQmx_Val_Rising, DAQmx_Val_ContSamps, numSamplesPerChannel ) ) )
				/*start the task*/
				retVal = DAQmxStartTask( *m_thDAQTask );						
	return retVal;
}

int32 ATIDAQHardwareInterface::ReadBufferedSamples( int numSamples, float64 buffer __gc[] )
{
	int32 retVal; /*the return value*/
	int32 sampsPerChannel = m_uiAveragingSize * numSamples; /*samples per channel*/
	unsigned int numRawSamples = sampsPerChannel * m_uiNumChannels; /*the total number of individual gauge readings
																	to be read*/
	float64 timeOut = ( sampsPerChannel / m_f64SamplingFrequency ) + 1; /*timeout value. allows a full second of
																		slack*/
	float64 __nogc* rawBuffer = new float64[ numRawSamples ];
	int32 read; /*number of samples read.*/
	unsigned int rawSetSize = m_uiNumChannels * m_uiAveragingSize; /*the number of raw data sets per one output set*/
	int i, j, k; /*generic loop/array indices*/
	retVal = DAQmxReadAnalogF64( *m_thDAQTask, sampsPerChannel, timeOut, DAQmx_Val_GroupByScanNumber, 
				rawBuffer, numRawSamples, &read, NULL );
	/*
	precondition: rawBuffer has the raw samples from the DAQ hardware.  rawSetSize is the number of
		data points in one output reading (one raw data point is a single reading of all 6 or 7 gauges).
	postcondition: buffer has the output (averaged) data points.  the first data point in each raw 'set' has the
		sum of all the readings in that set. i = numSamples, j = m_uiNumChannels, k = m_uiAveragingSize
	*/
	for ( i = 0; i < numSamples; i++ )
	{
		unsigned int firstDataPointInSetIndex = i * rawSetSize; /*the position of the first element
																in the first data point in the raw
																set we're currently averaging*/
		for ( j = 0; (unsigned int)j < m_uiNumChannels; j++ )
		{
			unsigned int sumIndex = firstDataPointInSetIndex + j; /*the index where we're storing
																  the sum of the gauge we're averaging
																  the raw values for*/
			for ( k = 1; (unsigned int)k < m_uiAveragingSize; k++ )
			{ /*put the sum of this set into the first reading in the set*/
                rawBuffer[ sumIndex ] += rawBuffer[ sumIndex + ( k * m_uiNumChannels ) ];			
			}
			
			/*put the averages into the output buffer*/
			buffer[ ( i * m_uiNumChannels ) + j ] = rawBuffer[ sumIndex ] / m_uiAveragingSize;
		}		
	}
	delete [] rawBuffer; /*keep up with those unmanaged pointers*/
	return retVal;
}

/*july.22.2005 - ss - added SetConnectionMode*/
void ATIDAQHardwareInterface::SetConnectionMode( int DAQConnMode )
{
	m_iConnectionMode = DAQConnMode;
}

/*july.22.2005 - ss - added GetConnectionMode*/
int ATIDAQHardwareInterface::GetConnectionMode( )
{
	return m_iConnectionMode;
}
