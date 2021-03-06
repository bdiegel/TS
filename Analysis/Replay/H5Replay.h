/* Copyright (C) 2010 Ion Torrent Systems, Inc. All Rights Reserved */
#ifndef H5REPLAY_H
#define H5REPLAY_H

#include <stdio.h>
#include <map>
#include <string>
#include <algorithm>
#include <vector>
#include <iostream>
#include "hdf5.h"
#include "CommandLineOpts.h"
#include "FileBkgReplay.h"
#include "IonErr.h"
#include "Utils.h"
#include <pthread.h>
#include <assert.h>

/**
 * Utility class to wrap up some of the complications of an 
 * HDF5 Dataset.  @TODO merge this utility class into H5
 */

class ReplayH5DataSet {

 public:
  ReplayH5DataSet();   // Constructor
  ~ReplayH5DataSet();  // Destructor

  void Close();        // Close all DataSet hdf5 handles if open
  void Clear();        // Initialize variables with empty state
 // set dataset name, type, rank
  void Init(const char *datasetname, hid_t datatype, int rank);

  // dataset has been set up so it can be created or accessed in the file
  bool IsInitialized() {return (mName==NULL ? false : true); };

  // dataset is created and exists in the file
  bool IsCreated(hid_t hFile);

  void Open(hid_t hFile);    // Open dataset hdf5 handles given file handle
  bool IsOpen();       // returns true if DataSet hdf5 handles are open

  unsigned int GetRank() { return( mRank ); };

  // Read a hyperslab of the hdf5 dataspace into buffer_out
  template<typename T>
    void Read(hsize_t *offset, hsize_t *count, unsigned int rank_out,
	      hsize_t *offset_out, hsize_t *count_out, T *buffer_out);

  // creates and opens all Dataset hdf5 handles assuming chunking
  void CreateDataset(hid_t hFile, hsize_t *chunk_dims);

  // extend the size of a DataSet's hdf5 dataspace
  void Extend(hsize_t *size);

  // writes buffer_in to file
  template<typename T>
    void Write(hsize_t *offset, hsize_t *count,
	       unsigned int rank_in, hsize_t *offset_in, hsize_t *count_in,
	       T *buffer_in);
  

  static const int EMPTY = -1;
  // std::string mGroup;  //**< Path to dataset in h5 file
  char *mName;   //**< Name of dataset

  /** Set the current gzip chunk compression level (0-9 valid only. */
  void SetCompression(int level) { mCompression = level; }
  int GetCompression() { return mCompression; }

 private:
  hid_t mDataset;      //**< H5 dataset handle
  hid_t mDatatype;     //**< H5 data definition
  hid_t mDataspace;    //**< handles
  hid_t mPList;        //** Property list handle
  hid_t mClass;  // data type class
  size_t mSize;        // size of data element
  int mRank;           // rank of data
  hsize_t mDims[3];    // dimensions, only supported up to 3
  hsize_t mChunkDims[3];  // chunk dimensions
  int mCompression;
  herr_t mStatus;      // status
  H5D_layout_t mLayout;// layout (i.e., chunked, contiguous, ...)

  bool mCreated;
  hsize_t maxdims[3];
};

// template read and write a dataset
template<typename T>
void ReplayH5DataSet::Read(hsize_t *offset, hsize_t *count,
			      unsigned int rank_out,
			      hsize_t *offset_out, hsize_t *count_out,
			      T *buffer_out)
{
  herr_t status;

  // the hyperslab in the file dataspace
  status = H5Sselect_hyperslab (mDataspace, H5S_SELECT_SET,
				       offset,NULL, count, NULL);
  assert(status>=0);

  // the hyperslab in memory
  hid_t memspace = H5Screate_simple(rank_out,count_out,NULL);
  status = H5Sselect_hyperslab (memspace, H5S_SELECT_SET,
				       offset_out,NULL, count_out, NULL);
  assert(status>=0);

  // now the actual read
  // fprintf(stdout, "H5: reading %s rank: %d, offset: [%d, %d, %d], size: [%d, %d, %d] from memory rank: %d, offset: [%d], size [%d]\n", mName, (int)mRank, (int)offset[0], (int)offset[1], (int)offset[2], (int)count[0], (int)count[1], (int)count[2], rank_out, (int)offset_out[0], (int)count_out[0]);
  status = H5Dread (mDataset, mDatatype, memspace, mDataspace,
		    H5P_DEFAULT, buffer_out);
  assert(status>=0);

  H5Sclose(memspace);
}

template<typename T>
void ReplayH5DataSet::Write(hsize_t *offset, hsize_t *count,
			       unsigned int rank_in,
			       hsize_t *offset_in, hsize_t *count_in,
			       T *buffer_in)
{
  // the hyperslab in the file dataspace
  herr_t status = H5Sselect_hyperslab (mDataspace, H5S_SELECT_SET,
				       offset,NULL, count, NULL);
  assert(status>=0);

  // the hyperslab in memory
  hid_t memspace = H5Screate_simple(rank_in, count_in, NULL);
  
  herr_t status1 = H5Sselect_hyperslab (memspace, H5S_SELECT_SET,
  				offset_in, NULL, count_in, NULL);
  assert(status1>=0);

  // now the actual write
  // fprintf(stdout, "H5: Writing %s rank: %d, offset: [%d, %d, %d], size: [%d, %d, %d] from memory rank: %d, offset: [%d], size [%d]\n", mName, mRank, offset[0], offset[1], offset[2], count[0], count[1], count[2], rank_in, offset_in[0], count_in[0]);

  herr_t status2 = H5Dwrite (mDataset, mDatatype, memspace, mDataspace,
		     H5P_DEFAULT, buffer_in);
  assert(status2>=0);

  H5Sclose(memspace);
}

// *********************************************************************
// reader and recorder

class H5Replay {
  // shared between both reader and recorder 

 public:
  // constructor
  H5Replay(CommandLineOpts& clo, char *datasetname);
  
  virtual ~H5Replay(); //Destructor

  void Close();  // Close all hdf5 handles

 protected:
  dsn mInfo;
  std::string mFilePath;
  hid_t mHFile;       ///< Id for hdf5 file operations.
  ReplayH5DataSet mRDataset; // we may want multiple datasets in the future

  void Init(CommandLineOpts& clo, char *datasetname); // initialize
  
  // the one and only file that houses the data
  void SetReplayBkgModelDataFile(CommandLineOpts &clo);

  static pthread_mutex_t h5_mutex;
  bool locked;

 private:
  H5Replay(); // unimplemented, don't call
};

// *********************************************************************
// reader

class H5ReplayReader : public H5Replay {

 public:
  // Constructor for just checking the file
  H5ReplayReader(CommandLineOpts& clo);
  
  // Constructor for reading a specific dataset
  H5ReplayReader(CommandLineOpts& clo, char *datasetname);
  void Open();        // Open file and dataset hdf5 handles

  template<typename T>
    void Read(std::vector<hsize_t>& offset, std::vector<hsize_t>& count,
	      std::vector<hsize_t>& offset_out, std::vector<hsize_t>& count_out,
	      T *buffer_out);
  
 private:
  bool CheckValid();

  H5ReplayReader(); // not implemented, don't call!
};

// template read from a dataset, open & close file
template<typename T>
void H5ReplayReader::Read(std::vector<hsize_t>& offset, std::vector<hsize_t>& count,
			  std::vector<hsize_t>& offset_out,
			  std::vector<hsize_t>& count_out, T *buffer_out)
{
  assert( mRDataset.GetRank() == offset.size());
  assert( mRDataset.GetRank() == count.size());
  assert( offset_out.size() == count_out.size());  

  Open();
  assert( mRDataset.IsCreated(mHFile) );
  mRDataset.Read(&offset[0], &count[0], offset_out.size(),
		 &offset_out[0], &count_out[0], buffer_out);
  Close();
}

// *********************************************************************
// recorder

class H5ReplayRecorder : public H5Replay {

public:  
  // Constructor for just creating the file
  H5ReplayRecorder(CommandLineOpts& clo);

  static bool fileNotCreated;  // since we only have one file possible
  void CreateFile();  // create the file to write datasets to


  // Constructor for creating or recording to a specific dataset
  H5ReplayRecorder(CommandLineOpts& clo, char *datasetname);

  void Open();        // Open file hdf5 (and if exists, dataset) handles

  //create an extensible dataset with given chunk size
  void CreateDataset(std::vector<hsize_t>& chunk_dims);
  void ExtendDataSet(std::vector<hsize_t>& extension);  // Extend a dataset

  // write buffer_in to the dataset
  template<typename T>
    void Write(std::vector<hsize_t>& offset, std::vector<hsize_t>& count,
	       std::vector<hsize_t>& offset_in, std::vector<hsize_t>& count_in,
	       T *buffer_in);

private:
  H5ReplayRecorder(); // not implemented, don't call!
};


// template write to a dataset, open & close file
template<typename T>
void H5ReplayRecorder::Write(std::vector<hsize_t>& offset, std::vector<hsize_t>& count,
			     std::vector<hsize_t>& offset_in,
			     std::vector<hsize_t>& count_in, T *buffer_in)
{
  assert( mRDataset.GetRank() == offset.size());
  assert( mRDataset.GetRank() == count.size());
  assert( count_in.size() == offset_in.size());

  Open();
  assert( mRDataset.IsCreated(mHFile) );
  mRDataset.Write(&offset[0], &count[0],
		  offset_in.size(), &offset_in[0], &count_in[0], buffer_in);
  Close();
}


#endif // H5REPLAY_H
