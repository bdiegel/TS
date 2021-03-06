/* Copyright (C) 2010 Ion Torrent Systems, Inc. All Rights Reserved */
#include "WellFileManipulation.h"
#include "Mask.h"

using namespace std;

void SetWellsToLiveBeadsOnly(RawWells &rawWells, Mask *maskPtr)
{
  // Get subset of wells we want to track, live only...
  vector<int> subset;
  size_t maskWells = maskPtr->H() * maskPtr->W();
  subset.reserve(maskWells);
  for (size_t i = 0; i < maskWells; i++) {
    if (maskPtr->Match(i, MaskLive)) {
      subset.push_back(i);
    }
  }
  rawWells.SetSubsetToWrite(subset);
}



void GetMetaDataForWells(char *explog_path, RawWells &rawWells, const char *chipType)
{
  const char * paramsToKeep[] = {"Project","Sample","Start Time","Experiment Name","User Name","Serial Number","Oversample","Frame Time", "Num Frames", "Cycles", "Flows", "LibraryKeySequence", "ChipTemperature", "PGMTemperature", "PGMPressure","W2pH","W1pH","Cal Chip High/Low/InRange"};
  char* paramVal = NULL;
  for (size_t pIx = 0; pIx < sizeof(paramsToKeep)/sizeof(char *); pIx++)
  {
    if ((paramVal = GetExpLogParameter(explog_path, paramsToKeep[pIx])) != NULL)
    {
      string value = paramVal;
      size_t pos = value.find_last_not_of("\n\r \t");
      if (pos != string::npos)
      {
        value = value.substr(0,pos+1);
      }
      rawWells.SetValue(paramsToKeep[pIx], value);
      free (paramVal);
    }

  }
  rawWells.SetValue("ChipType", chipType);
}



void CreateWellsFileForWriting (RawWells &rawWells, Mask *maskPtr,
                                CommandLineOpts &inception_state,
                                int num_fb,
                                int numFlows,
                                int numRows, int numCols,
                                const char *chipType)
{
  // set up wells data structure
  MemUsage ("BeforeWells");
  //rawWells.SetFlowChunkSize(flowChunk);
  rawWells.SetCompression (3);
  rawWells.SetRows (numRows);
  rawWells.SetCols (numCols);
  rawWells.SetFlows (numFlows);
  rawWells.SetFlowOrder (inception_state.flow_context.flowOrder); // 6th duplicated code
  SetWellsToLiveBeadsOnly (rawWells,maskPtr);
  // any model outputs a wells file of this nature
  GetMetaDataForWells (inception_state.sys_context.explog_path,rawWells,chipType);
  
  rawWells.OpenForWrite();
  rawWells.WriteRanks(); // dummy, written for completeness
  rawWells.WriteInfo();  // metadata written, do not need to rewrite
  rawWells.Close(); // just create in this routine
  MemUsage ("AfterWells");
}

void OpenExistingWellsForOneChunk(RawWells &rawWells,  int start_of_chunk, int chunk_depth)
{
    rawWells.SetChunk (0, rawWells.NumRows(), 0, rawWells.NumCols(), start_of_chunk, chunk_depth);
    rawWells.OpenForReadWrite();
}

void WriteOneChunkAndClose(RawWells &rawWells)
{
    rawWells.WriteWells();
    rawWells.Close();
}

// our logic here:  either we are writing an entire chunk_depth
// or we are writing to the end of the current block
int FigureChunkDepth(int flow, int numFlows, int write_well_flow_interval)
{
  return(min (write_well_flow_interval,numFlows-flow));
}

bool NeedToOpenWellChunk(int flow, int write_well_flow_interval)
{
  return( flow % write_well_flow_interval==0);
}



