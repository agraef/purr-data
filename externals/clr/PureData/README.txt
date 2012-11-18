to compile PureData.dll execute this command:

Using the Mono C#: 
mcs -unsafe+ -out:PureData.dll -target:library -optimize+ *.cs 

For the Microsoft C# compiler, use csc instead of mcs


put the PureData.dll anywhere in the PD search path (like e.g. the extra folder)

