Code Style Tool Guide:

================================================================================
TL;DR
    0. (Do this once for your system) Install cppcheck by downloading it from 
        the interwebs.
    1. Execute the run_[toolname].bat script from the desired project directory
       (e.g. ClearCore/run_cppcheck.bat). cppcheck puts its suggestions in 
       a file called issues.txt. Astyle just makes its changes without getting 
       your permission.

================================================================================
Tool Overview:

Astyle:     A syntax formatter that conforms to the C/C++ Style guide. (Yes,
            it corrects your files for you, so be careful)
            http://astyle.sourceforge.net/astyle.html

            Astyle edits files to conform to the Software team's coding styles.
            Its configuration file is in ClearCore/configuration. Astyle's
            purpose is to make the code for the project look like it was all
            written by one person. It does not change the meaning (semantics)
            of what's going on. The extent of its changes is purely cosmetic.
Cppcheck:   A semantic checker that makes sure you don't do something dumb
            like plowing off the end of an array, or tickling null pointers.
            http://cppcheck.sourceforge.net/
            
            NOTE: Cppcheck must be installed on your system by by downloading it 
            from the interwebs. The other application is a standalone exe file.

            Cppcheck doesn't care about how the code is formatted--that's
            Astyle's job. Cppcheck does not make any changes for you.
            It simply dumps its suggestions in a file called issues.txt 
            (in the same directory you execute run_cppcheck.bat from). 
            
================================================================================
