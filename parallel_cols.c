#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int size, my_rank;
MPI_Status status;

void binThreshold(int rows, int cols, int imageMatrix[rows][cols], int value){

    for(int i=0; i<rows; i++){
        for(int j=0; j<cols; j++){

            if (imageMatrix[i][j] < value)
                imageMatrix[i][j] = 1;
            else
                imageMatrix[i][j] = 0;
        }
    }
}


int **imageToMatrix(FILE *inputImage, int width, int height) {

    int **matrix = (int **)malloc(height * sizeof(int*));

    for( int i = 0; i < height; i++) {
        matrix[i] = (int *)malloc(width * sizeof(int));
    }

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (fscanf(inputImage, "%d", &matrix[i][j]) != 1) {
                // Handle error or unexpected end of file
                fprintf(stderr, "Error reading pixel values from the file.\n");
                exit(1);
            }
        }
    }

    return matrix;
}

void binComplement(int rows, int cols, int imageMatrix[rows][cols]){

    for(int i=0; i<rows; i++){
        for(int j=0; j<cols; j++){
            imageMatrix[i][j] = (imageMatrix[i][j] -1) * (-1);
        }
    }
}

void binErosion(int rows, int cols, int eroded[rows][cols]){

        int image[rows][cols];

        for (int i=0; i<rows; i++){
            for (int j=0; j<cols; j++){
                image[i][j]= eroded[i][j];
            }

        }
        
        for(int i = 1; i < rows -1 ; i++){
            for(int j = 1; j < cols -1; j++){
		
                if(image[i][j] * image[i+1][j] * image[i+1][j+1] *
                image[i][j+1] * image[i-1][j+1] * image[i-1][j] *
                image[i-1][j-1] * image[i][j-1] * image[i+1][j-1] != 0);
                
                else{
                            eroded[i][j] = 0;
                            eroded[i+1][j]= 0;
                            eroded[i+1][j+1]= 0;
                            eroded[i][j+1]= 0;
                            eroded[i-1][j+1]= 0;
                            eroded[i-1][j]= 0;
                            eroded[i-1][j-1]= 0;
                            eroded[i][j-1]= 0;
                            eroded[i+1][j-1]= 0; 
                    }
            }
        }    

}


void binDilation(int rows, int cols, int dilated[rows][cols]){


    int image[rows][cols];

    for (int i=0; i<rows; i++){
        for (int j=0; j<cols; j++){
            image[i][j]= dilated[i][j];
        }

    }

    for(int i = 1; i < rows -1 ; i++){
        for(int j = 1; j < cols -1; j++){
	
            if((image[i][j] + image[i+1][j] + image[i+1][j+1] +
            image[i][j+1] + image[i-1][j+1] + image[i-1][j] +
            image[i-1][j-1] + image[i][j-1] + image[i+1][j-1])==0);
                
            else{
                dilated[i][j] = 1;
                dilated[i+1][j]= 1;
                dilated[i+1][j+1]= 1;
                dilated[i][j+1]= 1;
                dilated[i-1][j+1]= 1;
                dilated[i-1][j]= 1;
                dilated[i-1][j-1]= 1;
                dilated[i][j-1]= 1;
                dilated[i+1][j-1]= 1; 
            }
	    }
    }   

}


void binOpening(int rows, int cols, int imageMatrix[rows][cols]){

    binErosion(rows, cols, imageMatrix);
    binDilation(rows, cols, imageMatrix);

}


void identifyBorders(int rows, int cols, int imageMatrix[rows][cols]) {

    int image[rows][cols];

        for (int i=0; i<rows; i++){
            for (int j=0; j<cols; j++){
                image[i][j]= imageMatrix[i][j];
            }

        }

    binErosion(rows, cols, imageMatrix);

    // Calculate the difference between the original and eroded images
    for(int i=0; i<rows; i++){
        for(int j=0; j<cols; j++){
            imageMatrix[i][j] = (imageMatrix[i][j] - image[i][j])*(-1);
        }
    }

}


void writeImage(int rows, int cols, int maxVal, int matrix[rows][cols], const char *outputFileName) {

    FILE *outputImage = fopen(outputFileName, "wb");

    if (outputImage == NULL) {
        printf("Error: Could not open %s for writing.\n", outputFileName);
        exit(1);
    }

    // Write PGM header
    fprintf(outputImage, "P2\n");
    fprintf(outputImage, "%d %d\n", cols, rows);
    fprintf(outputImage, "%d\n", maxVal); 

    // Write pixel values
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            
            fprintf(outputImage, "%d ", matrix[i][j]);
        }
        fprintf(outputImage, "\n");
    }

    fclose(outputImage);
}
    
  
int main(int argc, char*argv[]) {

    FILE *inputImage;
    char magicNumber[3];
    int width, height, maxVal, newRows, newHeight;
    double tot_time = 0.0;
    double start_time = 0.0;
    double stop_time = 0.0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Status status;

    start_time = MPI_Wtime();

    if (argc != 3) {
        fprintf(stderr, "Error: Wrong number of arguments\nUsage: mpirun -n <size> input_image.pgm <threshold>\n");
        return 1;
    }

    /* Opening the input image to be elaborated; if the fopen returns a NULL value, the opening of the file was not successful 
    *  and an error message is printed
    */
  
    inputImage = fopen(argv[1], "rb");
    if (inputImage == NULL) {
        printf("Error: %s can't be opened\n", argv[1]);
        return 1;
    }

    /* Reading the header of the input image; if the magic number of the header is different from P5 (magic number for the PGM 
     * file format), an error message is printed 
     */
    fscanf(inputImage, "%s", magicNumber);


    if (magicNumber[0] != 'P' || magicNumber[1] != '2') {
        printf("Error: file format not supported\n");
        return 1;
    }

    // Reading the width, height and maximum value contained in the header of the image
    fscanf(inputImage, "%d %d %d", &width, &height, &maxVal);

    int treshold=atoi(argv[2]);

    int result[height][width];
    int **imageMatrix;
        
    imageMatrix= imageToMatrix(inputImage, width, height);

     // Closing the file
    fclose(inputImage);
    
    // Synchronizing all processes before proceeding with computation 
    MPI_Barrier(MPI_COMM_WORLD);

    // Dividing the columns of the input image matrix among processes
    int colsPerProcess = width / size;
    int startCol = my_rank * colsPerProcess;
    int endCol = startCol + colsPerProcess;

    // Printing information about the range of columns each process is working on
    printf("Process %d working on columns %d to %d.\n", my_rank, startCol, endCol - 1);

    int recvMatrix[height][colsPerProcess];
  
    for (int j = 0; j < colsPerProcess; j++) {
        for (int i = 0; i < height; i++) {  
            recvMatrix[i][j] = imageMatrix[i][j + startCol];      
        } 
    }

    binThreshold(height, width/size, recvMatrix, treshold);

    binComplement(height, width/size, recvMatrix);

    binOpening(height, width/size, recvMatrix);
 

    for(int i=0; i<height; i++){
        for(int j=0; j<width/size; j++){
            recvMatrix[i][j] = recvMatrix[i][j]*255;
        }
    }


    if (my_rank == 0) {
            
        for(int i=0; i<height; i++){
            for(int j=0; j<width/size; j++){
                result[i][j] = recvMatrix[i][j];
            }
        }

        int processStartCol;
        int processEndCol;
        // Master process receives transformed columns from other processes
        for (int process = 1; process < size; process++) {
            MPI_Recv(&processStartCol, 1, MPI_INT, process, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&processEndCol, 1, MPI_INT, process, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int i = 0; i < height; i++) {
                MPI_Recv(&result[i][processStartCol], processEndCol-processStartCol, MPI_INT, process, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }
            
        } else {
            MPI_Send(&startCol, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
            MPI_Send(&endCol, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);
            // Slave processes send their transformed columns to the master process
            for (int i = 0; i < height; i++) {
                MPI_Send(&recvMatrix[i][startCol], endCol-startCol, MPI_INT, 0, i, MPI_COMM_WORLD);     
            }
        }


    if (my_rank==0){
        writeImage(height, width, maxVal, result, "./out/opened.pgm");        
    } 
      
    // Synchronizing all processes before proceeding with computation 
    MPI_Barrier(MPI_COMM_WORLD);
    
    identifyBorders(height, width, result);
    
    stop_time = MPI_Wtime();
    	
    if (my_rank==0){
        writeImage(height, width, maxVal, result, "./out/borders.pgm");
        tot_time= stop_time - start_time;
        printf("Total time: %f", tot_time);
    }


    MPI_Finalize();
    return 0;
}




/*

 mpicc parallel_cols.c -o parallel_cols

 mpirun -n 3 parallel_cols coins.pgm 80


*/
