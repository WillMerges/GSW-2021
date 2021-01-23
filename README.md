# GSW-2021
RIT Launch Initiative Ground Software for 2020-2021 \
\
\
To install in a new location or on a new machine: \
*chmod +x install* \
*./install* \
\
\
Before building or running the GSW environment needs to be set: \
*. setenv* \
(note: this *must* be run as *'. setenv'* and not *'./setenv'* because it needs to modify the current bash environment) \
\
\
How to build: \
*make all* \
\
How to clean: \
*make clean* \
\
\
There are different startup scripts located in *startup/* \
For example: \
    *cd startup/default* \
    *./start_gsw* \
    . \
    . \
    . run any GSW tools/applications \
    . \
    . \
    *./shutdown_gsw* \
\
note: do **not** alter or delete the *pidlist* file, it tracks what processes should be killed on shutdown
