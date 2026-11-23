@page sdk_guide_component 2. Implement a component

Derive the model wrapper from `HydroCouple::SDK::AbstractModelComponent`. Give each argument and exchange item a stable ID. A composition uses these IDs to set values and reconnect ports when it opens. The caption can describe the item in language familiar to model users.

### Declare and initialize
Use `createArguments()` to register configuration such as a model file or coefficients. In `initializeArguments(std::string& message)`, read the values, initialize the model and return a diagnostic on failure. Implement `initializeFailureCleanUp()` to release resources acquired before that failure. The base initialization sequence then calls `createInputs()` and `createOutputs()` on success.

### Validate and prepare
Once the host has connected the ports, `validate()` returns diagnostic messages. Check model-specific requirements here, such as the presence of a required inflow. In `prepare()`, allocate run state and make initial outputs available.

### Advance and finish
In `update()`, consume the inputs needed for the step, advance the underlying model, publish outputs and report the new status. Report `Done` when the model reaches its stopping condition. In `finish()`, close files and release run resources.

[Base class and lifecycle contract](https://github.com/HydroCouple/HydroCoupleSDK/blob/removing_qt/include/hydrocouplesdk/component/abstractmodelcomponent.h) · [Two complete component implementations](https://github.com/HydroCouple/HydroCoupleSDK/blob/removing_qt/examples/serial_coupling/main.cpp)
