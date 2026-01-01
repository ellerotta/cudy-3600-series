#!/bin/sh

# This script will repartition the bootfs and rootfs of the specified image partition.
# Note that the script only supports INCREASING bootfs size and DECREASING rootfs size.

####### FILL THIS SECTION BASED ON REQUIRED PARTITION GROWTH/REDUCTION ######### 
reqd_bootfs_size_megabytes="__MIN_BOOTFS_SIZE_MB__"
################################################################################

# Some constants
sleep_cnt_max=10
usage="   Usage: repart.sh <index of image that needs repartitioning [1|2]>" 
backup_ptable="/data/backup_ptable.bin"
repart_status="/data/repart_status.txt"
blk_dev_name="/dev/mmcblk0"
TEMP_FILE="/var/temp_dump"
repart_cmd_prefix="sgdisk"
repart_cmd_suffix="$blk_dev_name"
repart_add_cmd=""
repart_del_cmd=""
resize_app="blkpg-part"
resize_del_cmd_prefix="$resize_app delete $blk_dev_name"
resize_add_cmd_prefix="$resize_app add $blk_dev_name"
resize_add_cmd=""
resize_del_cmd=""

# Global variables
part_ent_name=""
part_ent_num=""
part_ent_startsec=""
part_ent_endsec=""
part_ent_sizesec=""


####################################################################
# gen_resize_cmd <part no> <part name> <start sector> <end sector> #
####################################################################
gen_partitioning_cmds () {
   part_num=$1
   part_name=$2
   part_startsec=$3
   part_endsec=$4
   part_startbytes=`expr $part_startsec \* ${sector_size}`
   part_sizesec=`expr $part_endsec - $part_startsec + 1`
   part_sizebytes=`expr $part_sizesec \* ${sector_size}`
   
   # Generate sgdisk commands
   repart_del_cmd="-d $part_num $repart_del_cmd"
   repart_add_cmd="-n $part_num:$part_startsec:$part_endsec -c $part_num:$part_name $repart_add_cmd"
   
   # Generate linux resizing commands
   if [ -z "$resize_del_cmd" ] ; then
      resize_del_cmd="$resize_del_cmd_prefix $part_num"
      resize_add_cmd="$resize_add_cmd_prefix $part_num $part_startbytes $part_sizebytes"
   else
      resize_del_cmd="$resize_del_cmd_prefix $part_num && $resize_del_cmd"
      resize_add_cmd="$resize_add_cmd_prefix $part_num $part_startbytes $part_sizebytes && $resize_add_cmd"
   fi
}

####################################################################
# parse_partition_entry  <partition entry line from sgdisk dump>   #
####################################################################
parse_partition_entry () {
   line=`echo $1`
   part_ent_name=${line##* }
   part_ent_num=${line%% *}
   part_ent_startsec=`expr "$line" : '\([0-9]* *[0-9]*\)'`
   part_ent_startsec=${part_ent_startsec##* }
   part_ent_endsec=`expr "$line" : '\([0-9]* *[0-9]* *[0-9]*\)'`
   part_ent_endsec=${part_ent_endsec##* }
   part_ent_sizesec=`expr $part_ent_endsec - $part_ent_startsec + 1`
   #echo "Partition entry line      : $line"
   #echo "Parsed partition values   : $part_ent_num/$part_ent_name/$part_ent_startsec/$part_ent_endsec/$part_ent_sizesec"
}

# Parse arguments and validate values
update_img_idx=$1
if [ -z "$reqd_bootfs_size_megabytes" ] || [ "$reqd_bootfs_size_megabytes" -eq "0" ]; then
   echo "repart.sh: ERROR! required bootfs size not specifed!"
   exit 1;
else
   reqd_bootfs_size_bytes=`expr $reqd_bootfs_size_megabytes \* 1048576`
fi

if [ -z "$update_img_idx" ]; then
   echo "repart.sh: ERROR! update image index not specified!" 
   exit 1;
else
   if [ "$update_img_idx" -ne "1" ] && [ "$update_img_idx" -ne "2" ]; then
      echo "repart.sh: ERROR! Invalid update image index $update_img_idx specified!" 
      echo "$usage"
      exit 1;
   fi
fi

# Store current device names
rootfs_ln_name=/dev/rootfs${update_img_idx}
rootfs_dev_name=`ls -al /dev/rootfs${update_img_idx} | cut -d '>' -f 2`
rootfs_dev_name=`echo ${rootfs_dev_name}`
bootfs_ln_name=/dev/bootfs${update_img_idx}
bootfs_dev_name=`ls -al /dev/bootfs${update_img_idx} | cut -d '>' -f 2`
bootfs_dev_name=`echo ${bootfs_dev_name}`

# Dump current partitions to a tempfile
rm -f $TEMP_FILE
sgdisk -p $blk_dev_name > $TEMP_FILE
if [ ! -e "$TEMP_FILE" ]; then
   echo "repart.sh: ERROR! Cannot dump partition config to $TEMP_FILE!"
   exit 1;
fi

# Get sector size
sector_size=`cat $TEMP_FILE | grep logical`
sector_size=`echo ${sector_size}`
sector_size=${sector_size##*: }
sector_size=${sector_size%%/*}

# Get required bootfs size in sectors
reqd_bootfs_size_sect=` expr $reqd_bootfs_size_bytes \/ ${sector_size}`


# Process bootfs
part_ent_line=`cat $TEMP_FILE | grep bootfs${update_img_idx}`
parse_partition_entry "$part_ent_line"

# Decide whether to grow or leave bootfs size
if [ "$part_ent_sizesec" -lt "$reqd_bootfs_size_sect" ]; then
   echo ""
   echo "Attempting to resize BOOTFS to atleast ${reqd_bootfs_size_megabytes}MB!"
   echo "Sector size (bytes)       : $sector_size"
   echo "Reqd bootfs size (bytes)  : $reqd_bootfs_size_bytes"
   echo "Reqd bootfs size (sect)   : $reqd_bootfs_size_sect"

   # Calculate size difference
   part_ent_sec_chng=`expr $reqd_bootfs_size_sect - $part_ent_sizesec`
   echo "Current bootfs size (sect): $part_ent_sizesec"
   echo "Current bootfs end sector : $part_ent_endsec"
   echo "End sector adjustment     : +$part_ent_sec_chng"
   
   # Adjust bootfs
   part_ent_endsec=`expr $part_ent_endsec + $part_ent_sec_chng`
   gen_partitioning_cmds $part_ent_num $part_ent_name $part_ent_startsec $part_ent_endsec
   echo "New bootfs end sector     : $part_ent_endsec"
   
   # Process rootfs
   part_ent_line=`cat $TEMP_FILE | grep rootfs${update_img_idx}`
   parse_partition_entry "$part_ent_line"
   echo "Current rootfs size (sect): $part_ent_sizesec"
   echo "Current rootfs strt sector: $part_ent_startsec"
   echo "Start sector adjustment   : +$part_ent_sec_chng"

   # Adjust rootfs
   part_ent_startsec=`expr $part_ent_startsec + $part_ent_sec_chng`
   gen_partitioning_cmds $part_ent_num $part_ent_name $part_ent_startsec $part_ent_endsec
   echo "New rootfs strt sector    : $part_ent_startsec"
else
   echo "repart.sh: Bootfs already resized to $reqd_bootfs_size_bytes bytes!" 
   exit 0;
fi

# Backup partition table
echo ""
echo "Backing up existing partition table ..."
sgdisk --backup=$backup_ptable $blk_dev_name
echo ""

# Create new partitioning commands
repart_cmd="$repart_cmd_prefix $repart_del_cmd $repart_add_cmd $repart_cmd_suffix"
resize_cmd="$resize_del_cmd && $resize_add_cmd"
echo "GPT repartitioning command:"
echo "$repart_cmd"
echo "Linux partition resizing commands:"
echo "$resize_del_cmd"
echo "$resize_add_cmd"

# Run sgdisk repartitioning commands
echo ""
echo "Running sgdisk repartitioning commands ..."
$repart_cmd
rc=$?
if [ $rc -ne 0 ]; then
   echo "repart.sh: ERROR! sgdisk repartitioning commands failed!"
fi

# Delete partition in linux
if [ $rc -eq 0 ]; then
   # Remove soft links
   \rm $rootfs_ln_name
   \rm $bootfs_ln_name

   # Resize partitions
   echo ""
   echo "Resizing partitions in linux - deleting partitions ..."
   eval "${resize_del_cmd}"
   rc=$?
   if [ $rc -ne 0 ]; then
      echo "repart.sh: ERROR! linux partition resizing delete commands failed!"
   else
      echo "Partition resizing delete commands completed successfully!"
   fi
fi

# Wait for partitions to be removed
if [ $rc -eq 0 ]; then
   # Wait and check if device nodes have been deleted 
   sleep_cnt=$sleep_cnt_max
   while [ -b $rootfs_dev_name ] || [ -b $bootfs_dev_name ]
   do
      if [ $sleep_cnt -eq 0 ]; then
          echo "repart.sh: ERROR! device node deletion timed out!"
          rc=1
          break
      fi
      sleep 1
      sleep_cnt=`expr $sleep_cnt - 1`
   done
   echo "sleep_cnt : $sleep_cnt"
fi

# Add partitions in linux
if [ $rc -eq 0 ]; then
   echo ""
   echo "Resizing partitions in linux - adding partitions ..."
   eval "${resize_add_cmd}"
   rc=$?
   if [ $rc -ne 0 ]; then
      echo "repart.sh: ERROR! linux partition resizing add commands failed!"
   else
      echo "Partition resizing add commands completed successfully!"
   fi
fi

#sleep 1 before creation of partions. Allow hotplug events to catchup
sleep 1

# Wait for partitions to be added
if [ $rc -eq 0 ]; then
   # Wait and check if device nodes exist 
   sleep_cnt=$sleep_cnt_max
   while [ ! -b $rootfs_dev_name ] || [ ! -b $bootfs_dev_name ] 
   do
      if [ $sleep_cnt -eq 0 ]; then
          echo "repart.sh: ERROR! device node link creation timed out!"
          rc=1
          break
      fi
      mdev -s
      sleep 1
      sleep_cnt=`expr $sleep_cnt - 1`
   done
   echo "sleep_cnt : $sleep_cnt"
fi

if [ $rc -eq 0 ]; then
   if [ ! -e $rootfs_ln_name ] || [ ! -e $bootfs_ln_name ]; then
       ln -s $rootfs_dev_name $rootfs_ln_name
       ln -s $bootfs_dev_name $bootfs_ln_name
   fi
   new_rootfs_size=`blockdev --getsize64 $rootfs_ln_name`
   new_bootfs_size=`blockdev --getsize64 $bootfs_ln_name`
fi

# Clear dentries,inode and page cache 
echo "Dropping dentries, page and inode caches"
sync; echo 3 > /proc/sys/vm/drop_caches

# Run recovery if needed 
if [ $rc -ne 0 ]; then
   echo ""
   echo "Restoring partitions from backup ..."
   sgdisk --load-backup=$backup_ptable $blk_dev_name
   echo "FAILED" > $repart_status
   exit 1
else
   echo "repart.sh: COMPLETE! New bootfs size=$new_bootfs_size, New rootfs size=$new_rootfs_size"
fi

echo "SUCCESS" > $repart_status
exit 0
