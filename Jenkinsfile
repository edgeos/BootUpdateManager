node {

    stage "SCM Checkout"
    checkout scm
    
    stage "Clean Existing Images"
    sh '''
        make all-clean
    '''
    
    stage "Pre-Build Steps"
    sh '''
        (cd ci-scripts && git pull) || git clone https://$(cat /home/jenkins/git_token)@github.build.ge.com/PredixEdge/ci-scripts.git
        source ci-scripts/prebuild.sh
    '''

    stage "Build & Image, all arch, build type: static library"
    sh '''
        make all-image BUILD_TYPE=static-library
        make all-bin-clean
        make all-image-clean
    '''

    stage "Build & Image, all arch, build type: shared library"
    sh '''
        make all-image BUILD_TYPE=shared-library
        make all-bin-clean
        make all-image-clean
    '''

    stage "Build & Image, all arch, build type: shared library"
    sh '''
        make all-image BUILD_TYPE=dynamic-library
        make all-bin-clean
        make all-image-clean
    '''

    stage "Build & Image, all arch, build type: executable"
    sh '''
        make all-image
        for im in $(cat .image-registry*) ; do docker run --rm $im ; done
        make all-bin-clean
        make all-image-clean
    '''

    stage "Test"
    sh '''
        make test
    '''

    stage "Scan"
    sh '''
        make all-bin-clean
        make scan
    '''

    stage "Post-Build Steps"
    sh '''
        source ci-scripts/postbuild.sh
    '''

    stage "Clean Existing Images"
    sh '''
        make clean
    '''

    stage "Final Step"
    sh '''
        source ci-scripts/finally.sh
    '''

}
