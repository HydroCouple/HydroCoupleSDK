# HydroCouple SDK

The SDK supplies C++20 implementations of the HydroCouple interfaces. Start with
@ref HydroCouple::SDK::AbstractModelComponent to wrap a model and
@ref HydroCouple::SDK::TimeSteppedWorkflow to schedule connected components.
Use @ref HydroCouple::SDK::ResultsModelComponent to read recorded output.

The @subpage sdk_guide follows a catchment and channel
through initialization, connection, execution and results review.
The [Composer user guide](../HydroCoupleComposer/guide/index.html) shows how
to load components and run a composition in the desktop application.
