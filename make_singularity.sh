#! /usr/bin/bash
SHORT=$(git rev-parse --short HEAD)
PWD=$(pwd)
DIR=$(basename "$PWD")

CPLEX_BIND=""
if [[ $1 == "cplex" ]]; then
    echo "Building CPLEX image"
    APPTAINER_FILE=ApptainerCPLEX
    export SINGULARITYENV_cplex_DIR=/cplex
    CPLEX_BIND="--bind /opt/ibm/ILOG/CPLEX_Studio2211/cplex:/cplex"
else
    echo "Building normal image"
    APPTAINER_FILE=Apptainer
fi

# Make the singularity builds directory if it doesn't exist
mkdir -p builds/singularity_builds

# Build the base build image if the definition file has changed
if [[ ApptainerBaseBuild -nt base_build.img ]]; then
    sudo singularity build "base_build.img" ApptainerBaseBuild
fi

# Build the base run image if the definition file has changed
if [[ ApptainerBaseRun -nt base_run.img ]]; then
    sudo singularity build "base_run.img" ApptainerBaseRun
fi

# Build the apptainer image if the definition file has changed
sudo singularity build $CPLEX_BIND\
    --bind "$PWD/src:/src" \
    --bind "$PWD/builds/singularity_builds:/builds" \
    "$DIR-$SHORT.img" $APPTAINER_FILE
