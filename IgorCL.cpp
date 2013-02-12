#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "cl.hpp"
#include <vector>

#include "IgorCL.h"
#include "IgorCLOperations.h"
#include "IgorCLUtilities.h"

// Global Variables (none)

// Runtime param structure for IGORCL operation.
#pragma pack(2)	// All structures passed to Igor are two-byte aligned.
struct IGORCLRuntimeParams {
	// Flag parameters.
    
	// Parameters for /PLTM flag group.
	int PLTMFlagEncountered;
	double PLTMFlag_platform;
	int PLTMFlagParamsSet[1];
    
	// Parameters for /DEV flag group.
	int DEVFlagEncountered;
	double DEVFlag_device;
	int DEVFlagParamsSet[1];
    
	// Parameters for /SRCT flag group.
	int SRCTFlagEncountered;
	Handle SRCTFlag_sourceText;
	int SRCTFlagParamsSet[1];
    
	// Parameters for /SRCB flag group.
	int SRCBFlagEncountered;
	waveHndl SRCBFlag_sourceBinary;
	int SRCBFlagParamsSet[1];
    
	// Parameters for /KERN flag group.
	int KERNFlagEncountered;
	Handle KERNFlag_kernelName;
	int KERNFlagParamsSet[1];
    
	// Parameters for /WGRP flag group.
	int WGRPFlagEncountered;
	double WGRPFlag_wgSize0;
	double WGRPFlag_wgSize1;
	double WGRPFlag_wgSize2;
	int WGRPFlagParamsSet[3];
    
	// Parameters for /RNG flag group.
	int RNGFlagEncountered;
	double RNGFlag_globalSize0;
	double RNGFlag_globalSize1;
	double RNGFlag_globalSize2;
	int RNGFlagParamsSet[3];
    
	// Main parameters.
    
	// Parameters for simple main group #0.
	int dataWavesEncountered;
	waveHndl dataWaves[12];					// Optional parameter.
	int dataWavesParamsSet[12];
    
	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct IGORCLRuntimeParams IGORCLRuntimeParams;
typedef struct IGORCLRuntimeParams* IGORCLRuntimeParamsPtr;
#pragma pack()	// Reset structure alignment to default.

// Runtime param structure for IgorCLCompile operation.
#pragma pack(2)	// All structures passed to Igor are two-byte aligned.
struct IgorCLCompileRuntimeParams {
	// Flag parameters.
    
	// Parameters for /PLTM flag group.
	int PLTMFlagEncountered;
	double PLTMFlag_platform;
	int PLTMFlagParamsSet[1];
    
	// Parameters for /DEV flag group.
	int DEVFlagEncountered;
	double DEVFlag_device;
	int DEVFlagParamsSet[1];
    
	// Parameters for /DEST flag group.
	int DESTFlagEncountered;
	DataFolderAndName DESTFlag_destination;
	int DESTFlagParamsSet[1];
    
	// Main parameters.
    
	// Parameters for simple main group #0.
	int programSourceEncountered;
	Handle programSource;
	int programSourceParamsSet[1];
    
	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct IgorCLCompileRuntimeParams IgorCLCompileRuntimeParams;
typedef struct IgorCLCompileRuntimeParams* IgorCLCompileRuntimeParamsPtr;
#pragma pack()	// Reset structure alignment to default.

// Runtime param structure for IGORCLInfo operation.
#pragma pack(2)	// All structures passed to Igor are two-byte aligned.
struct IGORCLInfoRuntimeParams {
	// Flag parameters.
    
	// Main parameters.
    
	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct IGORCLInfoRuntimeParams IGORCLInfoRuntimeParams;
typedef struct IGORCLInfoRuntimeParams* IGORCLInfoRuntimeParamsPtr;
#pragma pack()	// Reset structure alignment to default.

static int ExecuteIGORCL(IGORCLRuntimeParamsPtr p) {
	int err = 0;
    
    // Flag parameters.
    
    int platformIndex = 0;
	if (p->PLTMFlagEncountered) {
		// Parameter: p->PLTMFlag_platform
        if (p->PLTMFlag_platform < 0)
            return EXPECT_POS_NUM;
        platformIndex = p->PLTMFlag_platform + 0.5;
	}
    
    int deviceIndex = 0;
	if (p->DEVFlagEncountered) {
		// Parameter: p->DEVFlag_device
        if (p->DEVFlag_device < 0)
            return EXPECT_POS_NUM;
        deviceIndex = p->PLTMFlag_platform + 0.5;
	}
    
    bool sourceProvidedAsText = false;
    std::string textSource;
    if (p->SRCTFlagEncountered) {
		// Parameter: p->SRCTFlag_sourceText (test for NULL handle before using)
        if (p->SRCTFlag_sourceText == NULL)
            return USING_NULL_STRVAR;
        sourceProvidedAsText = true;
        textSource = GetStdStringFromHandle(p->SRCTFlag_sourceText);
	}
    
    waveHndl programBinaryWave = NULL;
    size_t nBytesInProgramBinary;
    std::vector<char> programBinary;
	if (p->SRCBFlagEncountered) {
		// Parameter: p->SRCBFlag_sourceBinary (test for NULL handle before using)
        if (p->SRCBFlag_sourceBinary == NULL)
            return NULL_WAVE_OP;
        if (sourceProvidedAsText)
            return GENERAL_BAD_VIBS;    // program needs to be provided as text OR binary
        
        programBinaryWave = p->SRCBFlag_sourceBinary;
        // require a wave containing bytes
        if (WaveType(programBinaryWave) != NT_I8)
            return NT_INCOMPATIBLE;
        // require that the wave is 1D
        int numDimensions;
        CountInt dimensionSizes[MAX_DIMENSIONS + 1];
        err = MDGetWaveDimensions(programBinaryWave, &numDimensions, dimensionSizes);
        if (err)
            return err;
        if (numDimensions != 1)
            return INCOMPATIBLE_DIMENSIONING;
        nBytesInProgramBinary = dimensionSizes[0];
        programBinary.resize(nBytesInProgramBinary);
        memcpy(reinterpret_cast<void*>(&programBinary[0]), WaveData(programBinaryWave), nBytesInProgramBinary);
	} else if (!sourceProvidedAsText) {
        return EXPECTED_STRING;         // program needs to be provided as text OR binary
    }
    
    std::string kernelName;
	if (p->KERNFlagEncountered) {
		// Parameter: p->KERNFlag_kernelName (test for NULL handle before using)
        if (p->KERNFlag_kernelName == NULL)
            return USING_NULL_STRVAR;
        kernelName = GetStdStringFromHandle(p->KERNFlag_kernelName);
	} else {
        return EXPECTED_STRING;
    }
    
    cl::NDRange globalRange;
	if (p->RNGFlagEncountered) {
		// Parameter: p->RNGFlag_globalSize0
		// Parameter: p->RNGFlag_globalSize1
		// Parameter: p->RNGFlag_globalSize2
        if ((p->RNGFlag_globalSize0 < 0) || (p->RNGFlag_globalSize1 < 0) || (p->RNGFlag_globalSize2 < 0))
            return EXPECT_POS_NUM;
        size_t gRange0 = p->RNGFlag_globalSize0 + 0.5;
        size_t gRange1 = p->RNGFlag_globalSize1 + 0.5;
        size_t gRange2 = p->RNGFlag_globalSize2 + 0.5;
        globalRange = cl::NDRange(gRange0, gRange1, gRange2);
	} else {
        return GENERAL_BAD_VIBS;
    }
    
    cl::NDRange workgroupSize;
    if (p->WGRPFlagEncountered) {
		// Parameter: p->WGRPFlag_wgSize0
		// Parameter: p->WGRPFlag_wgSize1
		// Parameter: p->WGRPFlag_wgSize2
        if ((p->WGRPFlag_wgSize0 < 0) || (p->WGRPFlag_wgSize1 < 0) || (p->WGRPFlag_wgSize2 < 0))
            return EXPECT_POS_NUM;
        size_t wRange0 = p->WGRPFlag_wgSize0 + 0.5;
        size_t wRange1 = p->WGRPFlag_wgSize1 + 0.5;
        size_t wRange2 = p->WGRPFlag_wgSize2 + 0.5;
        workgroupSize = cl::NDRange(wRange0, wRange1, wRange2);
	} else {
        workgroupSize = cl::NullRange;
    }
    
	// Main parameters.
    std::vector<waveHndl> waves;
    int nDataWaves = 0;
	if (p->dataWavesEncountered) {
		// Array-style optional parameter: p->dataWaves
		int* paramsSet = &p->dataWavesParamsSet[0];
		for(int i=0; i<12; i++) {
			if (paramsSet[i] == 0)
				break;		// No more parameters.
            if (p->dataWaves[i] == NULL)
                return NULL_WAVE_OP;
            
            waves.push_back(p->dataWaves[i]);
        }
    } else {
        return NOWAV;
    }
    
    try {
        if (sourceProvidedAsText) {
            DoOpenCLCalculation(platformIndex, deviceIndex, globalRange, workgroupSize, kernelName, waves, textSource);
        } else {
            DoOpenCLCalculation(platformIndex, deviceIndex, globalRange, workgroupSize, kernelName, waves, programBinary);
        }
    }
    catch (int e) {
        return e;
    }
    /*catch (cl::Error e) {
        XOPNotice(e.what());
        XOPNotice("\r");
        return GENERAL_BAD_VIBS;
    }*/
    catch (...) {
        return GENERAL_BAD_VIBS;
    }
    
	return err;
}

static int ExecuteIgorCLCompile(IgorCLCompileRuntimeParamsPtr p) {
	int err = 0;
    
	// Flag parameters.
    
    int platformIndex = 0;
	if (p->PLTMFlagEncountered) {
		// Parameter: p->PLTMFlag_platform
        platformIndex = p->PLTMFlag_platform + 0.5;
	}
    
    int deviceIndex = 0;
	if (p->DEVFlagEncountered) {
		// Parameter: p->DEVFlag_device
        deviceIndex = p->DEVFlag_device + 0.5;
	}
    
    DataFolderAndName destination;
	if (p->DESTFlagEncountered) {
		// Parameter: p->DESTFlag_destination
        destination = p->DESTFlag_destination;
	} else {
        destination.dfH = NULL;
        strcpy(destination.name, "W_CompiledBinary");
    }
    
	// Main parameters.
    
    std::string programSource;
	if (p->programSourceEncountered) {
		// Parameter: p->programSource (test for NULL handle before using)
        if (p->programSource == NULL)
            return EXPECTED_STRING;
        programSource = GetStdStringFromHandle(p->programSource);
	} else {
        return EXPECTED_STRING;
    }
    
    // do the actual compilation
    std::vector<char> compiledBinary;
    try {
        compiledBinary = compileSource(platformIndex, deviceIndex, programSource);
    }
    catch (int e) {
        return e;
    }
    catch (...) {
        return GENERAL_BAD_VIBS;
    }
    
    // copy the compiled binary back out to an Igor wave
    size_t nBytes = compiledBinary.size();
    CountInt dimensionSizes[MAX_DIMENSIONS + 1];
    dimensionSizes[0] = nBytes;
    dimensionSizes[1] = 0;
    waveHndl outputWave;
    err = MDMakeWave(&outputWave, destination.name, destination.dfH, dimensionSizes, NT_I8, 1);
    if (err)
        return err;
    
    memcpy(WaveData(outputWave), reinterpret_cast<void*>(&compiledBinary[0]), nBytes);
    
	return err;
}

static int ExecuteIGORCLInfo(IGORCLInfoRuntimeParamsPtr p) {
	int err = 0;
    cl_int status;
    
    char noticeString[256];
    
    try {
        
        // fetch information on the platforms
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);
        
        // create a text wave that will store the information
        waveHndl platformsWave;
        waveHndl devicesWave;
        CountInt dimensionSizes[MAX_DIMENSIONS + 1];
        IndexInt indices[MAX_DIMENSIONS];
        dimensionSizes[0] = 2;
        dimensionSizes[1] = platforms.size();
        dimensionSizes[2] = 0;
        Handle handle;
        err = MDMakeWave(&platformsWave, "M_OpenCLPlatforms", NULL, dimensionSizes, TEXT_WAVE_TYPE, 1);
        if (err)
            return err;
        
        cl_bool clBool;
        cl_ulong clUlong;
        // platform information
        for (int i = 0; i < platforms.size(); ++i) {
            indices[1] = i;
            indices[0] = 0;
            StoreStringInTextWave(platforms[i].getInfo<CL_PLATFORM_VENDOR>().c_str(), platformsWave, indices);
            indices[0] = 1;
            StoreStringInTextWave(platforms[i].getInfo<CL_PLATFORM_VERSION>().c_str(), platformsWave, indices);
            
            // device information
            std::vector<cl::Device> devices;
            status = platforms[i].getDevices(CL_DEVICE_TYPE_ALL, &devices);
            
            char deviceWaveName[50];
            sprintf(deviceWaveName, "M_OpenCLDevices%d", i);
            dimensionSizes[0] = 8;
            dimensionSizes[1] = devices.size();
            dimensionSizes[2] = 0;
            err = MDMakeWave(&devicesWave, deviceWaveName, NULL, dimensionSizes, TEXT_WAVE_TYPE, 1);
            if (err)
                return err;
            
            for (int j = 0; j < devices.size(); ++j) {
                indices[1] = j;
                indices[0] = 0;
                cl_device_type deviceType = devices[j].getInfo<CL_DEVICE_TYPE>();
                if (deviceType == CL_DEVICE_TYPE_CPU) {
                    StoreStringInTextWave("CPU", devicesWave, indices);
                } else if (deviceType == CL_DEVICE_TYPE_GPU) {
                    StoreStringInTextWave("GPU", devicesWave, indices);
                } else if (deviceType == CL_DEVICE_TYPE_ACCELERATOR) {
                    StoreStringInTextWave("Accelerator", devicesWave, indices);
                } else {
                    StoreStringInTextWave("Unknown", devicesWave, indices);
                }
                
                indices[0] = 1;
                StoreStringInTextWave(devices[j].getInfo<CL_DEVICE_NAME>(), devicesWave, indices);
                
                indices[0] = 2;
                StoreStringInTextWave(devices[j].getInfo<CL_DEVICE_VERSION>(), devicesWave, indices);
                
                indices[0] = 3;
                clBool = devices[j].getInfo<CL_DEVICE_AVAILABLE>();
                if (clBool) {
                    StoreStringInTextWave("device is available", devicesWave, indices);
                } else {
                    StoreStringInTextWave("device is unavailable", devicesWave, indices);
                }
                
                indices[0] = 4;
                clUlong = devices[j].getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>();
                sprintf(noticeString, "%lld", clUlong);
                StoreStringInTextWave(noticeString, devicesWave, indices);
                
                indices[0] = 5;
                sprintf(noticeString, "%lld", clUlong);
                StoreStringInTextWave(noticeString, devicesWave, indices);
                
                indices[0] = 6;
                clUlong = devices[j].getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>();
                sprintf(noticeString, "%lld", clUlong);
                StoreStringInTextWave(noticeString, devicesWave, indices);
                
                indices[0] = 7;
                StoreStringInTextWave(devices[j].getInfo<CL_DEVICE_EXTENSIONS>(), devicesWave, indices);
            }
        }
        
    }
    catch (...) {
        return GENERAL_BAD_VIBS;
    }
    
	return err;
}

static int RegisterIGORCL(void) {
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;
    
	// NOTE: If you change this template, you must change the IGORCLRuntimeParams structure as well.
	cmdTemplate = "IGORCL /PLTM=number:platform /DEV=number:device /SRCT=string:sourceText /SRCB=wave:sourceBinary /KERN=string:kernelName /WGRP={number:wgSize0, number:wgSize1, number:wgSize2} /RNG={number:globalSize0, number:globalSize1, number:globalSize2} wave[12]:dataWaves";
	runtimeNumVarList = "";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(IGORCLRuntimeParams), (void*)ExecuteIGORCL, 0);
}

static int RegisterIgorCLCompile(void) {
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;
    
	// NOTE: If you change this template, you must change the IgorCLCompileRuntimeParams structure as well.
	cmdTemplate = "IgorCLCompile /PLTM=number:platform /DEV=number:device /DEST=dataFolderAndName:destination string:programSource ";
	runtimeNumVarList = "";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(IgorCLCompileRuntimeParams), (void*)ExecuteIgorCLCompile, 0);
}

static int RegisterIGORCLInfo(void) {
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;
    
	// NOTE: If you change this template, you must change the IGORCLInfoRuntimeParams structure as well.
	cmdTemplate = "IGORCLInfo ";
	runtimeNumVarList = "";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(IGORCLInfoRuntimeParams), (void*)ExecuteIGORCLInfo, 0);
}

static int
RegisterOperations(void) {
	int result;
	
	// Register IgorCL operation.
	if (result = RegisterIGORCL())
		return result;
    if (result = RegisterIgorCLCompile())
        return result;
    if (result = RegisterIGORCLInfo())
        return result;
	
	// There are no more operations added by this XOP.
		
	return 0;
}

/*	XOPEntry()

	This is the entry point from the host application to the XOP for all
	messages after the INIT message.
*/
extern "C" void
XOPEntry(void)
{	
	XOPIORecResult result = 0;
	
	switch (GetXOPMessage()) {
		// We don't need to handle any messages for this XOP.
	}
	SetXOPResult(result);
}

/*	main(ioRecHandle)

	This is the initial entry point at which the host application calls XOP.
	The message sent by the host must be INIT.
	
	main does any necessary initialization and then sets the XOPEntry field of the
	ioRecHandle to the address to be called for future messages.
*/
HOST_IMPORT int
XOPMain(IORecHandle ioRecHandle)
{
	int result;
	
	XOPInit(ioRecHandle);				// Do standard XOP initialization.
	SetXOPEntry(XOPEntry);				// Set entry point for future calls.
	
	if (igorVersion < 600) {			// Requires Igor Pro 6.00 or later.
		SetXOPResult(OLD_IGOR);			// OLD_IGOR is defined in IgorCL.h and there are corresponding error strings in IgorCL.r and IgorCLWinCustom.rc.
		return EXIT_FAILURE;
	}

	if (result = RegisterOperations()) {
		SetXOPResult(result);
		return EXIT_FAILURE;
	}
	
	SetXOPResult(0);
	return EXIT_SUCCESS;
}
