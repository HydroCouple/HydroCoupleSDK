# HydroCoupleSDK

[![AUR](https://img.shields.io/aur/license/yaourt.svg)](https://github.com/HydroCouple/HydroCouple/blob/master/License.md)

The HydroCoupleSDK project is a software development kit that has been developed to simpilfy the model component development process for the HydroCouple component-based modeling interface definitions. It provides implementations of the core HydroCouple interfaces as well as implementations of various geotemporal data structures.

## Dependencies

HydroCoupleSDK can be compiled using the QtCreator project file included with this project or in visual studio using the Qt Visual Studio AddIn. Work is ongoing to add a CMake make file. Modify the project file to make sure that it points to the appropriate libraries on the target machine. Compilation of HydroCoupleSDK requires the following frameworks and libraries:

* Qt 5.x.x
* MPI
* OpenMP
* GDAL 2.2.2
* netcdf-cxx4 4.3.0
* hdf5 1.8.17


## References
-------------------------------------------------------------------------------------------------------------------
* Buahin, C. and J. Horsburgh, 2016. From OpenMI to HydroCouple: Advancing OpenMI to Support Experimental Simulations and Standard Geospatial Datasets. International Congress on Environmental Modelling and Software. Toulouse,          France. http://scholarsarchive.byu.edu/iemssconference/2016/Stream-A/11