#!/bin/bash

#SBATCH --account=PMIU0184
#SBATCH --job-name=image_search
#SBATCH --time=01:00:00
#SBATCH --mem=1GB
#SBATCH --nodes=1

export OMP_NUM_THREADS=$SLURM_NTASKS

/usr/bin/time -v ./homework6 images/Mammogram.png images/Cancer_mask.png result1.png
/usr/bin/time -v ./homework6 images/TestImage.png images/and_mask.png result2.png true 75 16
/usr/bin/time -v ./homework6 images/MiamiMarcumCenter.png images/WindowPane_mask.png result3.png true 50 64