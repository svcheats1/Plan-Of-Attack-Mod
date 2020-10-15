<p align="center">
  <img src="https://svcheats1.github.io/images/poa-logo.gif">
</p>

# Plan of Attack (Beta 4) Source Code
### April 15, 2006 -

One year ago, Plan of Attack was released as the first total conversion Half-Life 2 Mod. Today, on our first birthday, we are releasing our entire source code back to the community. We do this as a way to thank our dedicated community and to give other MOD teams an invaluable learning resource to use when creating new games.

We hope this gives aspiring MOD teams a huge step up in creating games for the Half-Life 2 engine. The art assets alone will save teams several man-months worth of development time. The source code includes all of our raw art assets for player models, weapons, textures, and props - all in Maya. Also included are all map source files, including a few unreleased maps.

The Plan of Attack Source Code is released under the Creative Commons license. This means anyone is able to copy and distribute modifications of this work, as long as they are non-commercial in nature and you credit the Plan of Attack Team.

We will be opening up <a href="https://web.archive.org/web/20130423152235/http://www.planofattackgame.com/forums/index.php?c=5">a new forum section dedicated to folks using our code base</a> to modify and create new MODs.

All files, totaling over 1 GB, can be found at <s>Filecloud</s> GameFront below:

   • Source Code (this page) - uncompiled source code

   • <s>Art Assets - raw art assets (Maya)</s>

   • <a href="https://www.gamefront.com/games/half-life-2/category/plan-of-attack">Full Development Copy</a> - raw maps and resources

# Notes
Only one modification was made to the code prior to release:
the encryption key for planofattack weapon files was removed at:
<b>game_shared/sdk/weapon_sdkbase.h,<b> <i>line 108</i>

You should create a new, random key for your mod to encrypt your 
weapon files with.
