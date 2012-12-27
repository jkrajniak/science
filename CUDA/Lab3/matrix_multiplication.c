// MP 3: Due Sunday, Dec 30, 2012 at 11:59 p.m. PST
// Author: Jakub Krajniak <jkrajniak@gmail.com>
// Assignment for https://class.coursera.org/hetero-2012-001/class/index

#include    <wb.h>

#define TILE_WIDTH 16

#define wbCheck(stmt) do {                                 \
        cudaError_t err = stmt;                            \
        if (err != cudaSuccess) {                          \
            wbLog(ERROR, "Failed to run stmt ", #stmt);    \
            return -1;                                     \
        }                                                  \
    } while(0)

// Compute C = A * B
__global__ void matrixMultiplyShared(float * A, float * B, float * C,
			             int numARows, int numAColumns,
			             int numBRows, int numBColumns,
			             int numCRows, int numCColumns) {
  
  	__shared__ float ds_M[TILE_WIDTH][TILE_WIDTH];
    __shared__ float ds_N[TILE_WIDTH][TILE_WIDTH];
  
    int bx = blockIdx.x; 
    int by = blockIdx.y;
    int tx = threadIdx.x; 
    int ty = threadIdx.y;
    
    int Row = by*TILE_WIDTH + ty;
    int Col = bx*TILE_WIDTH + tx;
  
    float Pvalue = 0;
    for (int m=0; m < (numAColumns-1)/TILE_WIDTH+1; ++m){
	    // load values into shared memory
	    // each thread has own ty, tx pair which load single value
	    // as every threads in block load all values into TILL_WIDTH memory, we can execute computing
	    // this part of matrix
	    if(Row < numARows && (m*TILE_WIDTH+tx) < numAColumns){
        		ds_M[ty][tx] = A[Row*numAColumns + m*TILE_WIDTH+tx];
        	} else {
          		ds_M[ty][tx] = 0;
	    }
	    if(m*TILE_WIDTH+ty < numBRows && Col < numBColumns){ 
		    ds_N[ty][tx] = B[(m*TILE_WIDTH+ty)*numBColumns+Col];
	    } else {
	        	ds_N[ty][tx] = 0;
	    }
          	__syncthreads();
	    if(Row < numCRows && Col < numCColumns){
            for(int k=0; k<TILE_WIDTH; ++k){
                Pvalue += ds_M[ty][k]*ds_N[k][tx];	  
            }
        }
        __syncthreads();
        if(Row < numCRows && Col < numCColumns){
            C[Row*numCColumns+Col] = Pvalue;
        }
    }
}

int main(int argc, char ** argv) {
    wbArg_t args;
    float * hostA; // The A matrix
    float * hostB; // The B matrix
    float * hostC; // The output C matrix
    float * deviceA;
    float * deviceB;
    float * deviceC;
    int numARows; // number of rows in the matrix A
    int numAColumns; // number of columns in the matrix A
    int numBRows; // number of rows in the matrix B
    int numBColumns; // number of columns in the matrix B
    int numCRows; // number of rows in the matrix C (you have to set this)
    int numCColumns; // number of columns in the matrix C (you have to set this)

    args = wbArg_read(argc, argv);

    wbTime_start(Generic, "Importing data and creating memory on host");
    hostA = (float *) wbImport(wbArg_getInputFile(args, 0), &numARows, &numAColumns);
    hostB = (float *) wbImport(wbArg_getInputFile(args, 1), &numBRows, &numBColumns);
    //@@ Set numCRows and numCColumns
    numCRows = numARows;
    numCColumns = numBColumns;
    //@@ Allocate the hostC matrix
	int sizeC = numCRows * numCColumns * sizeof(float);
  	int sizeA = numARows * numAColumns * sizeof(float);
  	int sizeB = numBRows * numBColumns * sizeof(float);
      
    hostC = (float *) malloc(sizeC);
    wbTime_stop(Generic, "Importing data and creating memory on host");

    wbLog(TRACE, "The dimensions of A are ", numAColumns, " x ", numARows);
    wbLog(TRACE, "The dimensions of B are ", numBColumns, " x ", numBRows);
    wbLog(TRACE, "The dimensions of C are ", numCColumns, " x ", numCRows);

    wbTime_start(GPU, "Allocating GPU memory.");
    //@@ Allocate GPU memory here
	cudaMalloc((void **) &deviceA, sizeA);
  	cudaMalloc((void **) &deviceB, sizeB);
  	cudaMalloc((void **) &deviceC, sizeC);
    wbTime_stop(GPU, "Allocating GPU memory.");

    wbTime_start(GPU, "Copying input memory to the GPU.");
    //@@ Copy memory to the GPU here
	cudaMemcpy(deviceA, hostA, sizeA, cudaMemcpyHostToDevice);
  	cudaMemcpy(deviceB, hostB, sizeB, cudaMemcpyHostToDevice);

    wbTime_stop(GPU, "Copying input memory to the GPU.");
    
    //@@ Initialize the grid and block dimensions here
    dim3 dimGrid(ceil(float(numCRows)/TILE_WIDTH), ceil(float(numCColumns)/TILE_WIDTH), 1);
    dim3 dimBlock(TILE_WIDTH, TILE_WIDTH, 1);
  	wbLog(TRACE, "Grid.x ", ceil(float(numCRows)/TILE_WIDTH), "Grid.y ", ceil(float(numCColumns)/TILE_WIDTH));
  
    wbTime_start(Compute, "Performing CUDA computation");
    //@@ Launch the GPU Kernel here
    wbLog(TRACE, "blockA ", ceil(float(numAColumns)/TILE_WIDTH));
    wbLog(TRACE, "blockB ", ceil(float(numBRows)/TILE_WIDTH));
  	
	matrixMultiplyShared<<<dimGrid, dimBlock>>>(deviceA, deviceB, deviceC,
                                                numARows, numAColumns,
                                                numBRows, numBColumns,
                                                numCRows, numCColumns);
    cudaThreadSynchronize();
    wbTime_stop(Compute, "Performing CUDA computation");
    
    wbTime_start(Copy, "Copying output memory to the CPU");
    //@@ Copy the GPU memory back to the CPU here
	cudaMemcpy(hostC, deviceC, sizeC, cudaMemcpyDeviceToHost);
    wbTime_stop(Copy, "Copying output memory to the CPU");

    wbTime_start(GPU, "Freeing GPU Memory");
    //@@ Free the GPU memory here
	cudaFree(deviceA);
  	cudaFree(deviceB);
  	cudaFree(deviceC);
    wbTime_stop(GPU, "Freeing GPU Memory");

    wbSolution(args, hostC, numCRows, numCColumns);

    free(hostA);
    free(hostB);
    free(hostC);

    return 0;
}