#/bin/bash
# A QM script to align all the permissions as the should be.
# Version 0.1
# Docs below licence

# Copyright (C) 2009  Diccon Tesson - Adrian Tesson Associates

# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
# ----

# Set the blow user and groups for this to work

# Groups, please set to your own system!
qmusername=""
qmusergroup=""
qmdev=""
qmadmin=""
# Will change this to use command line params later

if [ `id -u` != "0" ]; then
 echo "You must run this as root, or with sudo"
 exit 3
fi

# Validate groups
if [ $qmusername == "" -o $qmusergroup == "" -o $qmdev == "" -o $qmadmin == "" ]; then
  echo "None or not all groups have been set, please edite this script and enter correctly."
  exit 1
fi

# Move to parent qm directory
cd ../

# Perm mask reference
# 7 WRX
# 6 WR
# 5 RX
# 4 R

# Check this is a QM dir
if [ ! -e "./bin/qm" ]; then 
  echo "The directory $PWD does not appear to be a QM sys dir"
  exit 2
fi

# Set ownder as qm
chown -R $qmusername.$qmusergroup ./*

# Set all files to readble
chmod -R u+r ./*
chmod -R go=r ./*

# Set all top level dirs to readable and executable only
chmod 755 ./*

#init error as false
ERR=1

# All QM users (Devs too)
# Full access
chmod 775 \$IPC
chmod 665 \$IPC/*
chmod 775 \$FORMS
chmod 665 \$FORMS/*
if [ $? -ne 1 ]; then ERR=0; fi
chmod 775 \$LOGINS
chmod 665 \$LOGINS/*
chmod 775 errlog
chmod 665 errlog/*
if [ $? -ne 1 ]; then ERR=0; fi
chmod 775 temp
chmod 665 temp/*
if [ $? -ne 1 ]; then ERR=0; fi
chmod -R 755 bin

# QM Developers
chgrp -R $qmdev \$MAP
chmod 775 \$MAP
chmod 665 \$MAP/*
chgrp -R $qmdev gcat
chmod 775 gcat
chmod 664 gcat/*
chgrp -R $qmdev SYSCOM
chmod 755 SYSCOM
chmod 755 SYSCOM

# Admins only
chgrp -R $qmadmin \$HOLD
chmod 775 \$HOLD
chmod 665 \$HOLD/*
if [ $? -ne 1 ]; then ERR=0; fi
chgrp -R $qmadmin \$HOLD
chmod 775 \$HOLD.DIC
chmod 665 \$HOLD.DIC/*
chgrp -R $qmadmin \$HOLD
chmod 775 \$SVLISTS
chmod 665 \$SVLISTS/*
if [ $? -ne 1 ]; then ERR=0; fi

if [ $ERR ]; then 
  echo "Some of the above files may not yet exist. By all means run up QM and re run this command"
fi

# Tools
chmod -R 777 tools
#chmod -R 755 tools
chmod 755 buildgpl
