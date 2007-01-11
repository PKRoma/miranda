2003-02-03

A few things about the branches, the HEAD branch is now again the development
branch, v0-2-0 will have only bug fixes found in 0.2.0.0 (stable release)

This was not the case before the stable release of 0.2.0.0, 
where the v0-2-0 branch was the source of the quasi nightly cycle and
the HEAD branch had only bug fixes applied.

the main project workspace file can be found in miranda0100\miranda32\ you will need VC++

Developing FAQ (hah)

 Q: Where is the roadmap? what will come after 0.2.0.0?
 
 A: Look at miranda0100\todo.txt this gives a selection of features/ideas/functionality that 
    probably will make it into 3.xx,4.xx.

 Q: I've made some changes to the Miranda code! it's f00king kickA$$ an you have
    to use my changes! OR ALL YOU SUXX!

 A: Meh, if you make "most excellent" changes, please submit
    the changes to the patch tracker with a small statement declaring what the
    patch does -- please use diff rather than submitting entire source code files, 
    it is called a 'patch' after all.
 -

 Q: I've made some daring alterations that I know the Miranda development team won't want
    to add into the main release, can I compile and release my own version?

 A: Feel free, but beware! it maybe alright for you and a few of your friends to use
    but do not ship such binaries wholesale, we will get pissed off at you :) 
    We can not help people with their issues if they're using completely different version(s)
    to the version(s) we expect.

    We can not be held accountable for 3rd party binaries that may have been altered to
    to be backdoored/trojaned, et al.

 -

 Q: Where can I find a simple example of a Miranda plugin?

 A: Look in miranda0100\plugins\testplug, this is a very simple plugin

 -

 Q: Are you going to support Linux?

 A: Probably not, not a native version anyway, try using WINE -- you may end up having issues though.
 
 -

 Q: Which other compilers are supported?

 A: For the main source code, not very many! but plugin arch can be used by different
    compilers, including most C compilers, Delphi 2.0 thru 6.0 and FreePascal and probably with ASM 
    if you used the translation tools shipped with most assemblers (h2inc)
   
    compiler support for VB probably will NEVER happen!, 
    you can compile a native Win32 DLL with VB, look for "LinkSpoof" on www.vbaccelerator.com, 
    you will also need VC++, you have to write  all the interface code for Miranda in C++ and then 
    compile the source into an object module.

    After this, you must hijack the VB link process and link with your own object module.
    this must also force VB to use the .def file where you can define the three exports (Load, MirandaPluginInfo, Unload),
    once your code is loaded, you must give control to your VB code, a way of doing this
    is defining a native VB module with a simple interface, such as Public Sub Go() and 
    using your C++ code to 'extern' this function and letting the linker fix up the address,
    after all the VB module is compiled into a C++ object module anyway.

    Because Miranda passes a structure filled with function pointers, this is impossible
    to use from native VB (well not impossible) -- try and find an article/source code
    by Matt Curland which showed a method of hijacking the virtual method table of a native
    VB class to use as a prototype for calling another function pointer, this would enable
    you to call the function pointers, though there is a problem still! calling convention.

    COM uses STDCALL, Miranda uses CDECL, I don't know if you can use ODL/IDL to define 
    a type library class that you could create and then hijack -- you would be better off
    defining all the function pointers as static functions in your C++ source that called
    the function pointers when they were invoked by the VB module (of course you would need
    several VB dummy versions that were replaced by the C++ version at link).

    There is yet no script languages for Miranda (native or external) or ActiveScripting Support
    either! 

    You can however create a plugin that could invoke Perl (for example) but this isn't the 
    best way of doing things :)


 If you have any problems, please search the open discussion, plugin dev forums before
 asking a question that has probably been asked already: I know the search feature
 isn't that great, but try anyway!

 Note: Directly submitted files to any of the development team will be ignored 
       at the developers disgression (don't do it! :)
 

2001-04-27

Because people have been messing with the 0.0.7.0 code that is 2 months old,
and because I was tempted to due to the number of changes anyway, this
version is 0.1.0.0 (prerelease).

Fun things for users that you might want to check out:
- Interface rearrange
- File transfer
- Split mode message dialog plugin
- Contact list groups (these were in the old prerelease version)
- Autoaway built in
- User online alerts
- Great swathes of bug fixes

Not so fun things for users:
- The MSN support isn't supposed to work
- Can't use old plugins

Fun things for developers to check out:
- Insane modular organisation
- New database for storing everything (stick it on a floppy and you can
  be you on another machine)
- The opportunity to peruse my godawful code

BTW, about the MSN support: It'll be out until 0.1.1.0. Search for
'official roadmap' in the Open Discussion forum.

Stay slinky,

Richard.