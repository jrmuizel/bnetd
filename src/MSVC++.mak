# Microsoft Developer Studio Generated NMAKE File, Based on BNETD.dsp
!IF "$(CFG)" == ""
CFG=BNETD - Win32 Debug
!MESSAGE No configuration specified. Defaulting to BNETD - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "BNETD - Win32 Release" && "$(CFG)" != "BNETD - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "BNETD.mak" CFG="BNETD - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "BNETD - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "BNETD - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "BNETD - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\BNETD.exe"


CLEAN :
	-@erase "$(INTDIR)\account.obj"
	-@erase "$(INTDIR)\account_wrap.obj"
	-@erase "$(INTDIR)\adbanner.obj"
	-@erase "$(INTDIR)\addr.obj"
	-@erase "$(INTDIR)\autoupdate.obj"
	-@erase "$(INTDIR)\proginfo.obj"
	-@erase "$(INTDIR)\bits.obj"
	-@erase "$(INTDIR)\bits_chat.obj"
	-@erase "$(INTDIR)\bits_ext.obj"
	-@erase "$(INTDIR)\bits_game.obj"
	-@erase "$(INTDIR)\bits_login.obj"
	-@erase "$(INTDIR)\bits_motd.obj"
	-@erase "$(INTDIR)\bits_net.obj"
	-@erase "$(INTDIR)\bits_packet.obj"
	-@erase "$(INTDIR)\bits_query.obj"
	-@erase "$(INTDIR)\bits_rconn.obj"
	-@erase "$(INTDIR)\bits_va.obj"
	-@erase "$(INTDIR)\bn_type.obj"
	-@erase "$(INTDIR)\bnethash.obj"
	-@erase "$(INTDIR)\bnethashconv.obj"
	-@erase "$(INTDIR)\bnettime.obj"
	-@erase "$(INTDIR)\channel.obj"
	-@erase "$(INTDIR)\channel_conv.obj"
	-@erase "$(INTDIR)\character.obj"
	-@erase "$(INTDIR)\check_alloc.obj"
	-@erase "$(INTDIR)\command.obj"
	-@erase "$(INTDIR)\connection.obj"
	-@erase "$(INTDIR)\difftime.obj"
	-@erase "$(INTDIR)\eventlog.obj"
	-@erase "$(INTDIR)\file.obj"
	-@erase "$(INTDIR)\game.obj"
	-@erase "$(INTDIR)\game_conv.obj"
	-@erase "$(INTDIR)\game_rule.obj"
	-@erase "$(INTDIR)\gametrans.obj"
	-@erase "$(INTDIR)\gettimeofday.obj"
	-@erase "$(INTDIR)\give_up_root_privileges.obj"
	-@erase "$(INTDIR)\handle_auth.obj"
	-@erase "$(INTDIR)\handle_bits.obj"
	-@erase "$(INTDIR)\handle_bnet.obj"
	-@erase "$(INTDIR)\handle_bot.obj"
	-@erase "$(INTDIR)\handle_file.obj"
	-@erase "$(INTDIR)\handle_init.obj"
	-@erase "$(INTDIR)\handle_telnet.obj"
	-@erase "$(INTDIR)\handle_udp.obj"
	-@erase "$(INTDIR)\hashtable.obj"
	-@erase "$(INTDIR)\helpfile.obj"
	-@erase "$(INTDIR)\hexdump.obj"
	-@erase "$(INTDIR)\pdir.obj"
	-@erase "$(INTDIR)\inet_aton.obj"
	-@erase "$(INTDIR)\inet_ntoa.obj"
	-@erase "$(INTDIR)\ipban.obj"
	-@erase "$(INTDIR)\ladder.obj"
	-@erase "$(INTDIR)\ladder_calc.obj"
	-@erase "$(INTDIR)\list.obj"
	-@erase "$(INTDIR)\mail.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\memcpy.obj"
	-@erase "$(INTDIR)\memset.obj"
	-@erase "$(INTDIR)\message.obj"
	-@erase "$(INTDIR)\network.obj"
	-@erase "$(INTDIR)\packet.obj"
	-@erase "$(INTDIR)\prefs.obj"
	-@erase "$(INTDIR)\psock.obj"
	-@erase "$(INTDIR)\queue.obj"
	-@erase "$(INTDIR)\realm.obj"
	-@erase "$(INTDIR)\runprog.obj"
	-@erase "$(INTDIR)\server.obj"
	-@erase "$(INTDIR)\strcasecmp.obj"
	-@erase "$(INTDIR)\strdup.obj"
	-@erase "$(INTDIR)\strerror.obj"
	-@erase "$(INTDIR)\strftime.obj"
	-@erase "$(INTDIR)\strncasecmp.obj"
	-@erase "$(INTDIR)\strtoul.obj"
	-@erase "$(INTDIR)\tick.obj"
	-@erase "$(INTDIR)\timer.obj"
	-@erase "$(INTDIR)\token.obj"
	-@erase "$(INTDIR)\tracker.obj"
	-@erase "$(INTDIR)\udptest_send.obj"
	-@erase "$(INTDIR)\uname.obj"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\versiontest.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\watch.obj"
	-@erase "$(OUTDIR)\BNETD.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "..\src" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "HAVE_CONFIG_H" /Fp"$(INTDIR)\BNETD.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\BNETD.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=wsock32.lib kernel32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\BNETD.pdb" /machine:I386 /out:"$(OUTDIR)\BNETD.exe" 
LINK32_OBJS= \
	"$(INTDIR)\account.obj" \
	"$(INTDIR)\account_wrap.obj" \
	"$(INTDIR)\adbanner.obj" \
	"$(INTDIR)\addr.obj" \
	"$(INTDIR)\autoupdate.obj" \
	"$(INTDIR)\proginfo.obj" \
	"$(INTDIR)\bits.obj" \
	"$(INTDIR)\bits_chat.obj" \
	"$(INTDIR)\bits_ext.obj" \
	"$(INTDIR)\bits_game.obj" \
	"$(INTDIR)\bits_login.obj" \
	"$(INTDIR)\bits_motd.obj" \
	"$(INTDIR)\bits_net.obj" \
	"$(INTDIR)\bits_packet.obj" \
	"$(INTDIR)\bits_query.obj" \
	"$(INTDIR)\bits_rconn.obj" \
	"$(INTDIR)\bits_va.obj" \
	"$(INTDIR)\bn_type.obj" \
	"$(INTDIR)\bnethash.obj" \
	"$(INTDIR)\bnethashconv.obj" \
	"$(INTDIR)\bnettime.obj" \
	"$(INTDIR)\channel.obj" \
	"$(INTDIR)\channel_conv.obj" \
	"$(INTDIR)\character.obj" \
	"$(INTDIR)\check_alloc.obj" \
	"$(INTDIR)\command.obj" \
	"$(INTDIR)\connection.obj" \
	"$(INTDIR)\eventlog.obj" \
	"$(INTDIR)\file.obj" \
	"$(INTDIR)\game.obj" \
	"$(INTDIR)\game_conv.obj" \
	"$(INTDIR)\game_rule.obj" \
	"$(INTDIR)\gametrans.obj" \
	"$(INTDIR)\give_up_root_privileges.obj" \
	"$(INTDIR)\handle_auth.obj" \
	"$(INTDIR)\handle_bits.obj" \
	"$(INTDIR)\handle_bnet.obj" \
	"$(INTDIR)\handle_bot.obj" \
	"$(INTDIR)\handle_file.obj" \
	"$(INTDIR)\handle_init.obj" \
	"$(INTDIR)\handle_telnet.obj" \
	"$(INTDIR)\handle_udp.obj" \
	"$(INTDIR)\hashtable.obj" \
	"$(INTDIR)\helpfile.obj" \
	"$(INTDIR)\hexdump.obj" \
	"$(INTDIR)\ipban.obj" \
	"$(INTDIR)\ladder.obj" \
	"$(INTDIR)\ladder_calc.obj" \
	"$(INTDIR)\list.obj" \
	"$(INTDIR)\mail.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\message.obj" \
	"$(INTDIR)\network.obj" \
	"$(INTDIR)\packet.obj" \
	"$(INTDIR)\prefs.obj" \
	"$(INTDIR)\queue.obj" \
	"$(INTDIR)\realm.obj" \
	"$(INTDIR)\runprog.obj" \
	"$(INTDIR)\server.obj" \
	"$(INTDIR)\tick.obj" \
	"$(INTDIR)\timer.obj" \
	"$(INTDIR)\token.obj" \
	"$(INTDIR)\tracker.obj" \
	"$(INTDIR)\udptest_send.obj" \
	"$(INTDIR)\util.obj" \
	"$(INTDIR)\versioncheck.obj" \
	"$(INTDIR)\watch.obj" \
	"$(INTDIR)\strtoul.obj" \
	"$(INTDIR)\gettimeofday.obj" \
	"$(INTDIR)\pdir.obj" \
	"$(INTDIR)\inet_aton.obj" \
	"$(INTDIR)\inet_ntoa.obj" \
	"$(INTDIR)\memcpy.obj" \
	"$(INTDIR)\memset.obj" \
	"$(INTDIR)\psock.obj" \
	"$(INTDIR)\strcasecmp.obj" \
	"$(INTDIR)\strdup.obj" \
	"$(INTDIR)\strerror.obj" \
	"$(INTDIR)\strftime.obj" \
	"$(INTDIR)\strncasecmp.obj" \
	"$(INTDIR)\difftime.obj" \
	"$(INTDIR)\uname.obj"

"$(OUTDIR)\BNETD.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\BNETD.exe" "$(OUTDIR)\BNETD.bsc"


CLEAN :
	-@erase "$(INTDIR)\account.obj"
	-@erase "$(INTDIR)\account.sbr"
	-@erase "$(INTDIR)\account_wrap.obj"
	-@erase "$(INTDIR)\account_wrap.sbr"
	-@erase "$(INTDIR)\adbanner.obj"
	-@erase "$(INTDIR)\adbanner.sbr"
	-@erase "$(INTDIR)\addr.obj"
	-@erase "$(INTDIR)\addr.sbr"
	-@erase "$(INTDIR)\autoupdate.obj"
	-@erase "$(INTDIR)\autoupdate.sbr"
	-@erase "$(INTDIR)\proginfo.obj"
	-@erase "$(INTDIR)\proginfo.sbr"
	-@erase "$(INTDIR)\bits.obj"
	-@erase "$(INTDIR)\bits.sbr"
	-@erase "$(INTDIR)\bits_chat.obj"
	-@erase "$(INTDIR)\bits_chat.sbr"
	-@erase "$(INTDIR)\bits_ext.obj"
	-@erase "$(INTDIR)\bits_ext.sbr"
	-@erase "$(INTDIR)\bits_game.obj"
	-@erase "$(INTDIR)\bits_game.sbr"
	-@erase "$(INTDIR)\bits_login.obj"
	-@erase "$(INTDIR)\bits_login.sbr"
	-@erase "$(INTDIR)\bits_motd.obj"
	-@erase "$(INTDIR)\bits_motd.sbr"
	-@erase "$(INTDIR)\bits_net.obj"
	-@erase "$(INTDIR)\bits_net.sbr"
	-@erase "$(INTDIR)\bits_packet.obj"
	-@erase "$(INTDIR)\bits_packet.sbr"
	-@erase "$(INTDIR)\bits_query.obj"
	-@erase "$(INTDIR)\bits_query.sbr"
	-@erase "$(INTDIR)\bits_rconn.obj"
	-@erase "$(INTDIR)\bits_rconn.sbr"
	-@erase "$(INTDIR)\bits_va.obj"
	-@erase "$(INTDIR)\bits_va.sbr"
	-@erase "$(INTDIR)\bn_type.obj"
	-@erase "$(INTDIR)\bn_type.sbr"
	-@erase "$(INTDIR)\bnethash.obj"
	-@erase "$(INTDIR)\bnethash.sbr"
	-@erase "$(INTDIR)\bnethashconv.obj"
	-@erase "$(INTDIR)\bnethashconv.sbr"
	-@erase "$(INTDIR)\bnettime.obj"
	-@erase "$(INTDIR)\bnettime.sbr"
	-@erase "$(INTDIR)\channel.obj"
	-@erase "$(INTDIR)\channel.sbr"
	-@erase "$(INTDIR)\channel_conv.obj"
	-@erase "$(INTDIR)\channel_conv.sbr"
	-@erase "$(INTDIR)\character.obj"
	-@erase "$(INTDIR)\character.sbr"
	-@erase "$(INTDIR)\check_alloc.obj"
	-@erase "$(INTDIR)\check_alloc.sbr"
	-@erase "$(INTDIR)\command.obj"
	-@erase "$(INTDIR)\command.sbr"
	-@erase "$(INTDIR)\connection.obj"
	-@erase "$(INTDIR)\connection.sbr"
	-@erase "$(INTDIR)\difftime.obj"
	-@erase "$(INTDIR)\difftime.sbr"
	-@erase "$(INTDIR)\eventlog.obj"
	-@erase "$(INTDIR)\eventlog.sbr"
	-@erase "$(INTDIR)\file.obj"
	-@erase "$(INTDIR)\file.sbr"
	-@erase "$(INTDIR)\game.obj"
	-@erase "$(INTDIR)\game.sbr"
	-@erase "$(INTDIR)\game_conv.obj"
	-@erase "$(INTDIR)\game_conv.sbr"
	-@erase "$(INTDIR)\game_rule.obj"
	-@erase "$(INTDIR)\game_rule.sbr"
	-@erase "$(INTDIR)\gametrans.obj"
	-@erase "$(INTDIR)\gametrans.sbr"
	-@erase "$(INTDIR)\gettimeofday.obj"
	-@erase "$(INTDIR)\gettimeofday.sbr"
	-@erase "$(INTDIR)\give_up_root_privileges.obj"
	-@erase "$(INTDIR)\give_up_root_privileges.sbr"
	-@erase "$(INTDIR)\handle_auth.obj"
	-@erase "$(INTDIR)\handle_auth.sbr"
	-@erase "$(INTDIR)\handle_bits.obj"
	-@erase "$(INTDIR)\handle_bits.sbr"
	-@erase "$(INTDIR)\handle_bnet.obj"
	-@erase "$(INTDIR)\handle_bnet.sbr"
	-@erase "$(INTDIR)\handle_bot.obj"
	-@erase "$(INTDIR)\handle_bot.sbr"
	-@erase "$(INTDIR)\handle_file.obj"
	-@erase "$(INTDIR)\handle_file.sbr"
	-@erase "$(INTDIR)\handle_init.obj"
	-@erase "$(INTDIR)\handle_init.sbr"
	-@erase "$(INTDIR)\handle_telnet.obj"
	-@erase "$(INTDIR)\handle_telnet.sbr"
	-@erase "$(INTDIR)\handle_udp.obj"
	-@erase "$(INTDIR)\handle_udp.sbr"
	-@erase "$(INTDIR)\hashtable.obj"
	-@erase "$(INTDIR)\hashtable.sbr"
	-@erase "$(INTDIR)\helpfile.obj"
	-@erase "$(INTDIR)\helpfile.sbr"
	-@erase "$(INTDIR)\hexdump.obj"
	-@erase "$(INTDIR)\hexdump.sbr"
	-@erase "$(INTDIR)\pdir.obj"
	-@erase "$(INTDIR)\pdir.sbr"
	-@erase "$(INTDIR)\inet_aton.obj"
	-@erase "$(INTDIR)\inet_aton.sbr"
	-@erase "$(INTDIR)\inet_ntoa.obj"
	-@erase "$(INTDIR)\inet_ntoa.sbr"
	-@erase "$(INTDIR)\ipban.obj"
	-@erase "$(INTDIR)\ipban.sbr"
	-@erase "$(INTDIR)\ladder.obj"
	-@erase "$(INTDIR)\ladder.sbr"
	-@erase "$(INTDIR)\ladder_calc.obj"
	-@erase "$(INTDIR)\ladder_calc.sbr"
	-@erase "$(INTDIR)\list.obj"
	-@erase "$(INTDIR)\list.sbr"
	-@erase "$(INTDIR)\mail.obj"
	-@erase "$(INTDIR)\mail.sbr"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\main.sbr"
	-@erase "$(INTDIR)\memcpy.obj"
	-@erase "$(INTDIR)\memcpy.sbr"
	-@erase "$(INTDIR)\memset.obj"
	-@erase "$(INTDIR)\memset.sbr"
	-@erase "$(INTDIR)\message.obj"
	-@erase "$(INTDIR)\message.sbr"
	-@erase "$(INTDIR)\network.obj"
	-@erase "$(INTDIR)\network.sbr"
	-@erase "$(INTDIR)\packet.obj"
	-@erase "$(INTDIR)\packet.sbr"
	-@erase "$(INTDIR)\prefs.obj"
	-@erase "$(INTDIR)\prefs.sbr"
	-@erase "$(INTDIR)\psock.obj"
	-@erase "$(INTDIR)\psock.sbr"
	-@erase "$(INTDIR)\queue.obj"
	-@erase "$(INTDIR)\queue.sbr"
	-@erase "$(INTDIR)\realm.obj"
	-@erase "$(INTDIR)\realm.sbr"
	-@erase "$(INTDIR)\runprog.obj"
	-@erase "$(INTDIR)\runprog.sbr"
	-@erase "$(INTDIR)\server.obj"
	-@erase "$(INTDIR)\server.sbr"
	-@erase "$(INTDIR)\strcasecmp.obj"
	-@erase "$(INTDIR)\strcasecmp.sbr"
	-@erase "$(INTDIR)\strdup.obj"
	-@erase "$(INTDIR)\strdup.sbr"
	-@erase "$(INTDIR)\strerror.obj"
	-@erase "$(INTDIR)\strerror.sbr"
	-@erase "$(INTDIR)\strftime.obj"
	-@erase "$(INTDIR)\strftime.sbr"
	-@erase "$(INTDIR)\strncasecmp.obj"
	-@erase "$(INTDIR)\strncasecmp.sbr"
	-@erase "$(INTDIR)\strtoul.obj"
	-@erase "$(INTDIR)\strtoul.sbr"
	-@erase "$(INTDIR)\tick.obj"
	-@erase "$(INTDIR)\tick.sbr"
	-@erase "$(INTDIR)\timer.obj"
	-@erase "$(INTDIR)\timer.sbr"
	-@erase "$(INTDIR)\token.obj"
	-@erase "$(INTDIR)\token.sbr"
	-@erase "$(INTDIR)\tracker.obj"
	-@erase "$(INTDIR)\tracker.sbr"
	-@erase "$(INTDIR)\udptest_send.obj"
	-@erase "$(INTDIR)\udptest_send.sbr"
	-@erase "$(INTDIR)\uname.obj"
	-@erase "$(INTDIR)\uname.sbr"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\util.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\versioncheck.obj"
	-@erase "$(INTDIR)\versioncheck.sbr"
	-@erase "$(INTDIR)\watch.obj"
	-@erase "$(INTDIR)\watch.sbr"
	-@erase "$(OUTDIR)\BNETD.bsc"
	-@erase "$(OUTDIR)\BNETD.exe"
	-@erase "$(OUTDIR)\BNETD.ilk"
	-@erase "$(OUTDIR)\BNETD.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MLd /W3 /Gm /ZI /Od /I "..\src" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "HAVE_CONFIG_H" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\BNETD.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\BNETD.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\account.sbr" \
	"$(INTDIR)\account_wrap.sbr" \
	"$(INTDIR)\adbanner.sbr" \
	"$(INTDIR)\addr.sbr" \
	"$(INTDIR)\autoupdate.sbr" \
	"$(INTDIR)\proginfo.sbr" \
	"$(INTDIR)\bits.sbr" \
	"$(INTDIR)\bits_chat.sbr" \
	"$(INTDIR)\bits_ext.sbr" \
	"$(INTDIR)\bits_game.sbr" \
	"$(INTDIR)\bits_login.sbr" \
	"$(INTDIR)\bits_motd.sbr" \
	"$(INTDIR)\bits_net.sbr" \
	"$(INTDIR)\bits_packet.sbr" \
	"$(INTDIR)\bits_query.sbr" \
	"$(INTDIR)\bits_rconn.sbr" \
	"$(INTDIR)\bits_va.sbr" \
	"$(INTDIR)\bn_type.sbr" \
	"$(INTDIR)\bnethash.sbr" \
	"$(INTDIR)\bnethashconv.sbr" \
	"$(INTDIR)\bnettime.sbr" \
	"$(INTDIR)\channel.sbr" \
	"$(INTDIR)\character.sbr" \
	"$(INTDIR)\check_alloc.sbr" \
	"$(INTDIR)\command.sbr" \
	"$(INTDIR)\connection.sbr" \
	"$(INTDIR)\eventlog.sbr" \
	"$(INTDIR)\file.sbr" \
	"$(INTDIR)\game.sbr" \
	"$(INTDIR)\game_conv.sbr" \
	"$(INTDIR)\game_rule.sbr" \
	"$(INTDIR)\gametrans.sbr" \
	"$(INTDIR)\give_up_root_privileges.sbr" \
	"$(INTDIR)\handle_auth.sbr" \
	"$(INTDIR)\handle_bits.sbr" \
	"$(INTDIR)\handle_bnet.sbr" \
	"$(INTDIR)\handle_bot.sbr" \
	"$(INTDIR)\handle_file.sbr" \
	"$(INTDIR)\handle_init.sbr" \
	"$(INTDIR)\handle_telnet.sbr" \
	"$(INTDIR)\handle_udp.sbr" \
	"$(INTDIR)\hashtable.sbr" \
	"$(INTDIR)\helpfile.sbr" \
	"$(INTDIR)\hexdump.sbr" \
	"$(INTDIR)\ipban.sbr" \
	"$(INTDIR)\ladder.sbr" \
	"$(INTDIR)\ladder_calc.sbr" \
	"$(INTDIR)\list.sbr" \
	"$(INTDIR)\mail.sbr" \
	"$(INTDIR)\main.sbr" \
	"$(INTDIR)\message.sbr" \
	"$(INTDIR)\network.sbr" \
	"$(INTDIR)\packet.sbr" \
	"$(INTDIR)\prefs.sbr" \
	"$(INTDIR)\queue.sbr" \
	"$(INTDIR)\realm.sbr" \
	"$(INTDIR)\runprog.sbr" \
	"$(INTDIR)\server.sbr" \
	"$(INTDIR)\tick.sbr" \
	"$(INTDIR)\timer.sbr" \
	"$(INTDIR)\token.sbr" \
	"$(INTDIR)\tracker.sbr" \
	"$(INTDIR)\udptest_send.sbr" \
	"$(INTDIR)\util.sbr" \
	"$(INTDIR)\versioncheck.sbr" \
	"$(INTDIR)\watch.sbr" \
	"$(INTDIR)\strtoul.sbr" \
	"$(INTDIR)\gettimeofday.sbr" \
	"$(INTDIR)\pdir.sbr" \
	"$(INTDIR)\inet_aton.sbr" \
	"$(INTDIR)\inet_ntoa.sbr" \
	"$(INTDIR)\memcpy.sbr" \
	"$(INTDIR)\memset.sbr" \
	"$(INTDIR)\psock.sbr" \
	"$(INTDIR)\strcasecmp.sbr" \
	"$(INTDIR)\strdup.sbr" \
	"$(INTDIR)\strerror.sbr" \
	"$(INTDIR)\strftime.sbr" \
	"$(INTDIR)\strncasecmp.sbr" \
	"$(INTDIR)\difftime.sbr" \
	"$(INTDIR)\uname.sbr"

"$(OUTDIR)\BNETD.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=wsock32.lib kernel32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\BNETD.pdb" /debug /machine:I386 /out:"$(OUTDIR)\BNETD.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\account.obj" \
	"$(INTDIR)\account_wrap.obj" \
	"$(INTDIR)\adbanner.obj" \
	"$(INTDIR)\addr.obj" \
	"$(INTDIR)\autoupdate.obj" \
	"$(INTDIR)\proginfo.obj" \
	"$(INTDIR)\bits.obj" \
	"$(INTDIR)\bits_chat.obj" \
	"$(INTDIR)\bits_ext.obj" \
	"$(INTDIR)\bits_game.obj" \
	"$(INTDIR)\bits_login.obj" \
	"$(INTDIR)\bits_motd.obj" \
	"$(INTDIR)\bits_net.obj" \
	"$(INTDIR)\bits_packet.obj" \
	"$(INTDIR)\bits_query.obj" \
	"$(INTDIR)\bits_rconn.obj" \
	"$(INTDIR)\bits_va.obj" \
	"$(INTDIR)\bn_type.obj" \
	"$(INTDIR)\bnethash.obj" \
	"$(INTDIR)\bnethashconv.obj" \
	"$(INTDIR)\bnettime.obj" \
	"$(INTDIR)\channel.obj" \
	"$(INTDIR)\channel_conv.obj" \
	"$(INTDIR)\character.obj" \
	"$(INTDIR)\check_alloc.obj" \
	"$(INTDIR)\command.obj" \
	"$(INTDIR)\connection.obj" \
	"$(INTDIR)\eventlog.obj" \
	"$(INTDIR)\file.obj" \
	"$(INTDIR)\game.obj" \
	"$(INTDIR)\game_conv.obj" \
	"$(INTDIR)\game_rule.obj" \
	"$(INTDIR)\gametrans.obj" \
	"$(INTDIR)\give_up_root_privileges.obj" \
	"$(INTDIR)\handle_auth.obj" \
	"$(INTDIR)\handle_bits.obj" \
	"$(INTDIR)\handle_bnet.obj" \
	"$(INTDIR)\handle_bot.obj" \
	"$(INTDIR)\handle_file.obj" \
	"$(INTDIR)\handle_init.obj" \
	"$(INTDIR)\handle_telnet.obj" \
	"$(INTDIR)\handle_udp.obj" \
	"$(INTDIR)\hashtable.obj" \
	"$(INTDIR)\helpfile.obj" \
	"$(INTDIR)\hexdump.obj" \
	"$(INTDIR)\ipban.obj" \
	"$(INTDIR)\ladder.obj" \
	"$(INTDIR)\ladder_calc.obj" \
	"$(INTDIR)\list.obj" \
	"$(INTDIR)\mail.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\message.obj" \
	"$(INTDIR)\network.obj" \
	"$(INTDIR)\packet.obj" \
	"$(INTDIR)\prefs.obj" \
	"$(INTDIR)\queue.obj" \
	"$(INTDIR)\realm.obj" \
	"$(INTDIR)\runprog.obj" \
	"$(INTDIR)\server.obj" \
	"$(INTDIR)\tick.obj" \
	"$(INTDIR)\timer.obj" \
	"$(INTDIR)\token.obj" \
	"$(INTDIR)\tracker.obj" \
	"$(INTDIR)\udptest_send.obj" \
	"$(INTDIR)\util.obj" \
	"$(INTDIR)\versioncheck.obj" \
	"$(INTDIR)\watch.obj" \
	"$(INTDIR)\strtoul.obj" \
	"$(INTDIR)\gettimeofday.obj" \
	"$(INTDIR)\pdir.obj" \
	"$(INTDIR)\inet_aton.obj" \
	"$(INTDIR)\inet_ntoa.obj" \
	"$(INTDIR)\memcpy.obj" \
	"$(INTDIR)\memset.obj" \
	"$(INTDIR)\psock.obj" \
	"$(INTDIR)\strcasecmp.obj" \
	"$(INTDIR)\strdup.obj" \
	"$(INTDIR)\strerror.obj" \
	"$(INTDIR)\strftime.obj" \
	"$(INTDIR)\strncasecmp.obj" \
	"$(INTDIR)\difftime.obj" \
	"$(INTDIR)\uname.obj"

"$(OUTDIR)\BNETD.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("BNETD.dep")
!INCLUDE "BNETD.dep"
!ELSE 
!MESSAGE Warning: cannot find "BNETD.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "BNETD - Win32 Release" || "$(CFG)" == "BNETD - Win32 Debug"
SOURCE=..\src\bnetd\account.c

!IF  "$(CFG)" == "BNETD - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX- /O2 /I "..\src" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "HAVE_CONFIG_H" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\account.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX- /ZI /Od /I "..\src" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "HAVE_CONFIG_H" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\account.obj"	"$(INTDIR)\account.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=..\src\bnetd\account_wrap.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\account_wrap.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\account_wrap.obj"	"$(INTDIR)\account_wrap.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\adbanner.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\adbanner.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\adbanner.obj"	"$(INTDIR)\adbanner.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\common\addr.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\addr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\addr.obj"	"$(INTDIR)\addr.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\autoupdate.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\autoupdate.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\autoupdate.obj"	"$(INTDIR)\autoupdate.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\proginfo.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\proginfo.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\proginfo.obj"	"$(INTDIR)\proginfo.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\bits.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\bits.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\bits.obj"	"$(INTDIR)\bits.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\bits_chat.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\bits_chat.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\bits_chat.obj"	"$(INTDIR)\bits_chat.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\bits_ext.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\bits_ext.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\bits_ext.obj"	"$(INTDIR)\bits_ext.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\bits_game.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\bits_game.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\bits_game.obj"	"$(INTDIR)\bits_game.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\bits_login.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\bits_login.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\bits_login.obj"	"$(INTDIR)\bits_login.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\bits_motd.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\bits_motd.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\bits_motd.obj"	"$(INTDIR)\bits_motd.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\bits_net.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\bits_net.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\bits_net.obj"	"$(INTDIR)\bits_net.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\bits_packet.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\bits_packet.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\bits_packet.obj"	"$(INTDIR)\bits_packet.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\bits_query.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\bits_query.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\bits_query.obj"	"$(INTDIR)\bits_query.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\bits_rconn.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\bits_rconn.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\bits_rconn.obj"	"$(INTDIR)\bits_rconn.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\bits_va.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\bits_va.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\bits_va.obj"	"$(INTDIR)\bits_va.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\common\bn_type.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\bn_type.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\bn_type.obj"	"$(INTDIR)\bn_type.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\common\bnethash.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\bnethash.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\bnethash.obj"	"$(INTDIR)\bnethash.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\common\bnethashconv.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\bnethashconv.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\bnethashconv.obj"	"$(INTDIR)\bnethashconv.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\common\bnettime.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\bnettime.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\bnettime.obj"	"$(INTDIR)\bnettime.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\channel.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\channel.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\channel.obj"	"$(INTDIR)\channel.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\channel_conv.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\channel_conv.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\channel_conv.obj"	"$(INTDIR)\channel_conv.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\character.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\character.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\character.obj"	"$(INTDIR)\character.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\common\check_alloc.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\check_alloc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\check_alloc.obj"	"$(INTDIR)\check_alloc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\command.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\command.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\command.obj"	"$(INTDIR)\command.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\connection.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\connection.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\connection.obj"	"$(INTDIR)\connection.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\compat\difftime.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\difftime.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\difftime.obj"	"$(INTDIR)\difftime.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\common\eventlog.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\eventlog.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\eventlog.obj"	"$(INTDIR)\eventlog.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\file.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\file.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\file.obj"	"$(INTDIR)\file.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\game.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\game.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\game.obj"	"$(INTDIR)\game.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\game_conv.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\game_conv.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\game_conv.obj"	"$(INTDIR)\game_conv.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\game_rule.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\game_rule.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\game_rule.obj"	"$(INTDIR)\game_rule.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\gametrans.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\gametrans.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\gametrans.obj"	"$(INTDIR)\gametrans.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\compat\gettimeofday.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\gettimeofday.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\gettimeofday.obj"	"$(INTDIR)\gettimeofday.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\common\give_up_root_privileges.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\give_up_root_privileges.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\give_up_root_privileges.obj"	"$(INTDIR)\give_up_root_privileges.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\handle_auth.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\handle_auth.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\handle_auth.obj"	"$(INTDIR)\handle_auth.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\handle_bits.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\handle_bits.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\handle_bits.obj"	"$(INTDIR)\handle_bits.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\handle_bnet.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\handle_bnet.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\handle_bnet.obj"	"$(INTDIR)\handle_bnet.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\handle_bot.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\handle_bot.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\handle_bot.obj"	"$(INTDIR)\handle_bot.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\handle_file.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\handle_file.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\handle_file.obj"	"$(INTDIR)\handle_file.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\handle_init.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\handle_init.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\handle_init.obj"	"$(INTDIR)\handle_init.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\handle_telnet.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\handle_telnet.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\handle_telnet.obj"	"$(INTDIR)\handle_telnet.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\handle_udp.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\handle_udp.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\handle_udp.obj"	"$(INTDIR)\handle_udp.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\common\hashtable.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\hashtable.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\hashtable.obj"	"$(INTDIR)\hashtable.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\helpfile.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\helpfile.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\helpfile.obj"	"$(INTDIR)\helpfile.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\common\hexdump.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\hexdump.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\hexdump.obj"	"$(INTDIR)\hexdump.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\compat\pdir.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\pdir.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\pdir.obj"	"$(INTDIR)\pdir.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\compat\inet_aton.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\inet_aton.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\inet_aton.obj"	"$(INTDIR)\inet_aton.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\compat\inet_ntoa.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\inet_ntoa.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\inet_ntoa.obj"	"$(INTDIR)\inet_ntoa.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\ipban.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\ipban.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\ipban.obj"	"$(INTDIR)\ipban.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\ladder.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\ladder.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\ladder.obj"	"$(INTDIR)\ladder.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\ladder_calc.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\ladder_calc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\ladder_calc.obj"	"$(INTDIR)\ladder_calc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\common\list.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\list.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\list.obj"	"$(INTDIR)\list.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\mail.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\mail.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\mail.obj"	"$(INTDIR)\mail.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\main.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\main.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\main.obj"	"$(INTDIR)\main.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\compat\memcpy.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\memcpy.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\memcpy.obj"	"$(INTDIR)\memcpy.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\compat\memset.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\memset.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\memset.obj"	"$(INTDIR)\memset.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\message.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\message.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\message.obj"	"$(INTDIR)\message.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\common\network.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\network.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\network.obj"	"$(INTDIR)\network.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\common\packet.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\packet.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\packet.obj"	"$(INTDIR)\packet.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\prefs.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\prefs.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\prefs.obj"	"$(INTDIR)\prefs.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\compat\psock.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\psock.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\psock.obj"	"$(INTDIR)\psock.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\common\queue.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\queue.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\queue.obj"	"$(INTDIR)\queue.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\realm.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\realm.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\realm.obj"	"$(INTDIR)\realm.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\runprog.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\runprog.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\runprog.obj"	"$(INTDIR)\runprog.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\server.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\server.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\server.obj"	"$(INTDIR)\server.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\compat\strcasecmp.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\strcasecmp.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\strcasecmp.obj"	"$(INTDIR)\strcasecmp.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\compat\strdup.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\strdup.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\strdup.obj"	"$(INTDIR)\strdup.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\compat\strerror.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\strerror.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\strerror.obj"	"$(INTDIR)\strerror.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\compat\strftime.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\strftime.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\strftime.obj"	"$(INTDIR)\strftime.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\compat\strncasecmp.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\strncasecmp.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\strncasecmp.obj"	"$(INTDIR)\strncasecmp.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\compat\strtoul.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\strtoul.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\strtoul.obj"	"$(INTDIR)\strtoul.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\tick.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\tick.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\tick.obj"	"$(INTDIR)\tick.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\timer.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\timer.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\timer.obj"	"$(INTDIR)\timer.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\common\token.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\token.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\token.obj"	"$(INTDIR)\token.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\tracker.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\tracker.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\tracker.obj"	"$(INTDIR)\tracker.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\udptest_send.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\udptest_send.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\udptest_send.obj"	"$(INTDIR)\udptest_send.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\compat\uname.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\uname.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\uname.obj"	"$(INTDIR)\uname.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\common\util.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\util.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\util.obj"	"$(INTDIR)\util.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\versioncheck.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\versioncheck.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\versioncheck.obj"	"$(INTDIR)\versioncheck.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bnetd\watch.c

!IF  "$(CFG)" == "BNETD - Win32 Release"


"$(INTDIR)\watch.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "BNETD - Win32 Debug"


"$(INTDIR)\watch.obj"	"$(INTDIR)\watch.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


!ENDIF 

