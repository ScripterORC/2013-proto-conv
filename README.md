# 2013-proto-conv

Hey, this is Scripter's new Github account. Lost access to the last one.
I'm also the person who made the ROBLOX Version Downloader (Revamped) a couple of years ago.

Recently I've been experimenting with Lua and old ROBLOX clients, which led me to this project: 2013-proto-conv (..or Sexine)

This is a "proto conversion" exploit that allows *unsigned Lua script execution on a certain range of ROBLOX clients.* Right now, this source is built for a certain 2013 client because that's what I had on hand at the time, though I will not name which.

Proto conversion is, in a nutshell, taking a Vanilla function prototype (Proto), converting it to Roblox's format, and executing it. This was the standard method of execution for many well-known modern ROBLOX script execution exploits until ROBLOX implemented LuaU, their new VM.

Custom anticheats aside, a client this old doesn't require nearly as much effort to exploit, since there was practically no security surrounding ROBLOX's Lua implementation at the time (thus there was no real difference between a ROBLOX proto and a Lua proto). Actually, if you really wanted to exploit on old ROBLOX, simply getting a lua state, calling luaL_loadbuffer, and executing the resulting function with something like lua_pcall, lua_call, or lua_resume would suffice.

However, I decided to go the extra mile anyways and find a different method of execution because it sounded super fun. This repository is the final result of that.

This project took me a week to develop (..though 2 days were for the actual execution and the other 5 were for ironing out various bugs). It's remarkably stable in my opinion, and if you know how to update it, should serve you well.

I'm still new to Github, so I'll have to find out how to set the correct license for this thing, but you have my full permission to modify, use, and distribute this source code. You don't have to credit me, though I'd like it if you did.

***HOWEVER:***

**I do not condone the usage of this project for anything malicious.** Please don't ruin some poor kid's revival because you luckily found a source that worked on Github.
If someone's using this on your revival, contact me using my Discord. It's at the bottom of this page.

Instead, **I strongly urge you to use this project for learning purposes.** Understand what this program does, and maybe even do some experimenting yourself.

You will have to update this exploit to work with a different client, though that shouldn't be that hard if you know what you're doing. I'm going to assume you do if you found this.

The only things you have to update are the addresses of the Lua functions, the hook locations, and (if the client is new enough) certain offsets.

My code is (somewhat) annotated to help you follow it, but I'll still give a brief rundown on how it works:

1. Place exception hooks for 1. obtaining the lua state 2. redirecting Roblox output to the exploit console window 3. trustcheck.
To the uninformed, exception hooks ""inject"" code into relevant functions by causing them to raise an exception, catching it with a custom "Vectored Exception Handler", and running our own code afterwards. More on that in the source code. (by the way, take the word "inject" with a grain of salt. That may not be the most accurate way to describe it, but I am trying to be simple.)

NOTE: For reference, the Lua state is G(L)->mainthread, or the Lua state's associated global state's main thread. This is because using the Lua state as is can mess up the script that it was from.

2. Set the current identity to 7 so that the scripts we execute have unrestricted access to ROBLOX's environment (can get/set restricted fields like a Player's name, call functions like game:HttpGet, and so on)

3. Once the lua state has been obtained, handle execution. Execution is done by:
  1. Creating a new thread from the ROBLOX Lua state we got
  2. Compiling the script in vanilla Lua.
  3. Acquiring the resulting Proto (function prototype).
  4. Converting it to a ROBLOX Proto (though it's literally just luaF_newproto and filling in the important members)
  5. Creating a new LClosure in ROBLOX
		5a. Setting the LClosure's Proto member to our converted Proto
		5b. Populating the LClosure's table of upvalues for the function to use.
  6. Pushing the LClosure onto the stack
  7. Calling the LClosure with lua_resume
  8. Cleaning up the stack
  
Note that my annotations are not nearly as professional as this README, but should still get the point across.

That's it. If you want to contact me, my Discord's John#0698. Peace
