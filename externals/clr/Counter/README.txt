to compile External.dll put  PureData.dll in this folder then execute this command:

Using the Mono C#: 
mcs -out:Counter.dll -target:library -r:PureData.dll -optimize+ *.cs

For the Microsoft C# compiler, use csc instead of mcs
