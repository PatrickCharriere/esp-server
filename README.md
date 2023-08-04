# esp-server

## Setup you machine 
To get the repo locally, first add the ssh key of your machine to your github account: https://docs.github.com/en/authentication/connecting-to-github-with-ssh/adding-a-new-ssh-key-to-your-github-account

## Install the tools
Then Install Visual Studio code and PlatformIO IDE plugin. You might need to restart VSCode several times to complete this installation process. During this installation process, PIO should install all the required plugins to solve the dependencies in main.cpp.

## Build and upload the image
Once this step in completed you can go to the PlatformIO tab in VSCode and click on "Build Filesystem Image" and should obtain this result in the console:
```
================================================================ [SUCCESS] Took 6.26 seconds ================================================================
 *  Terminal will be reused by tasks, press any key to close it.
```

Finally, you can connect your ESP-32 board to your laptop and try to "Upload Filesystem Image" 
```
=============================================================== [SUCCESS] Took 23.52 seconds ===============================================================
 *  Terminal will be reused by tasks, press any key to close it.
```
