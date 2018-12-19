## Test environments
* local ubunutu 18.01 LTS install, R 3.5.1
* win-builder (devel and release)

## R CMD check results

* local ubunutu 18.01 LTS install, R 3.5.1
0 errors | 0 warnings | 0 notes

* win-builder (devel and release)
Status: 1 NOTE
- This is a new release.

>> Can you pls make the Description more aligned with and informative than 
>> the Title?
>> Also, pls write the Description in sentence style.

- Updated DESCRIPTION file based on previous comments

>>Thanks, we see:
>> 
>>    The Description field should not start with the package name,
>>      'This package' or similar.
>> 
>> And indeed, "The package cfepi" is redundant and should beomitted.
>> Note that software names should also be single quoted in the Description 
>> field.
>> 
>> Please fix and resubmit.

- Should be fixed.

>> 
>> Is there some reference about the method you can add in the Description 
>> field in the form Authors (year) <doi:.....>?

- I have put in a preprint.  The intent is for the package to proceed the release of the paper.

>> The Description field contains
>>   details available at: <https://doi.org/10.1101/451153>.
>> Please write DOIs as <doi:10.prefix/suffix>.
>>
>> Please fix and resubmit.

- Misread the doi format.  Should be fixed now.

## Reverse dependencies

This is a new release, so there are no reverse dependencies.
