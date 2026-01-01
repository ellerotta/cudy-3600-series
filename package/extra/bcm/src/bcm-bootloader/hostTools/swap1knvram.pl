#!/usr/bin/env perl


use bytes;
use Getopt::Long;
use FindBin qw[$Bin];
use lib "$Bin/PerlLib";

use warnings;
use strict;




use Digest::CRC;

my $iname = shift @ARGV;
open(F,"<$iname");
local $/;
my $img = <F>;
close(F);
my $nname = shift @ARGV;
open(F,"<$nname");
my $nvtext = <F>;
close(F);
$nvtext = join('',split(/\s+/,$nvtext));
$nvtext =~ s/^.*?0600/0600/;
$nvtext =~ s/#.*$//;
$nvtext = pack('H*',$nvtext);

for (my $i = 0 ; $i < 1024 ; $i++) {
    for (my $j = 0 ; $j <= 0x580 ; $j +=  0x580) {
	my $a = $i * 1024 + $j;
        my $nvram = substr($img, $a, 1024);
        my @nvdata = unpack("V*",$nvram);
	if (($nvdata[0] == 0x4172566e) && ($nvdata[1] == 0x5461446d)) {
		$a = $a + 8;
        	$nvram = substr($img, $a, 1024);
                @nvdata = unpack("V*",$nvram);
	}
	next unless ($nvdata[0] == 6);
	my $orig_chsum=$nvdata[255];
	my $ctx = new Digest::CRC(width => 32, poly => 0x04c11db7, init => 0xffffffff, xorout => 0, refin => 1, refout => 1);
	my $data=substr($nvram,0,1020).pack("HHHH",0,0,0,0);
	$ctx->add($data);
	my $csum=$ctx->digest;
	if($orig_chsum == $csum)
	{
		printf("%x %x\n",$a,$nvdata[0]);
		print "length was " . length($img) .  "\n";
		substr($img, $a,1024,$nvtext);
		print "length is " . length($img) .  "\n";
	}
    }
}

open(F,">new_$iname");
print F $img;
close(F);

=pod
#initialize the data structure
my $ulNvramLen=$c->sizeof('NVRAM_DATA');
my $data= "\377" x $ulNvramLen;
my $dstruct=$c->unpack('NVRAM_DATA', $data);
$dstruct->{'szVoiceBoardId'}="\000";
#$dstruct->{'szBootline'}="\000";

#go through the command line arguments
#createimg --set boardid=963138REF_P502 voiceboardid=  numbermac=11 macaddr=02:10:18:01:00:01 tp=0 psisize=48 logsize= auxfsprcnt= gponsn= gponpw= inputfile=bcm963138BGWV_cfe_fs_kernel outputfile=bcm963138BGWV_flash_image_963138REF_P502
#
#

#set defaults
$dstruct->{'ulCheckSum'}=0;
$dstruct->{'ulVersion'}=6;
$dstruct->{'ulPsiSize'}=$psi_size;
$dstruct->{'backupPsi'}=$backupPsi;
$dstruct->{'ulSyslogSize'}=$logSize;
$dstruct->{'ucFlashBlkSize'}=$flashBlkSize;

# NO NEW NVRAM IMAGES:
# NO NEW NVRAM IMAGES:    foreach (@alloptions) {
# NO NEW NVRAM IMAGES:		my $carg=$_;
# NO NEW NVRAM IMAGES:		my @pair=split(/=/, $carg);
# NO NEW NVRAM IMAGES:		my $arg=$pair[0];
# NO NEW NVRAM IMAGES:		$arg=~s/^--//;
# NO NEW NVRAM IMAGES:		my $fp=$arg_mapper->{$arg};
# NO NEW NVRAM IMAGES:		if(ref($fp))
# NO NEW NVRAM IMAGES:		{
# NO NEW NVRAM IMAGES:			$fp->($dstruct, $pair[1]);
# NO NEW NVRAM IMAGES:		}
# NO NEW NVRAM IMAGES:		else
# NO NEW NVRAM IMAGES:		{
# NO NEW NVRAM IMAGES:			if(exists($dstruct->{$arg}))
# NO NEW NVRAM IMAGES:			{
# NO NEW NVRAM IMAGES:				$dstruct->{$arg}=$pair[1];
# NO NEW NVRAM IMAGES:			}
# NO NEW NVRAM IMAGES:		}
# NO NEW NVRAM IMAGES:
# NO NEW NVRAM IMAGES:        }
my $binary = $c->pack('NVRAM_DATA', $dstruct);
my $ctx = new Digest::CRC(width => 32, poly => 0x04c11db7, init => 0xffffffff, xorout => 0, refin => 1, refout => 1);
$ctx->add($binary);
my $csum=$ctx->digest;


$dstruct->{'ulCheckSum'}=$csum;
my $nvram_binary = $c->pack('NVRAM_DATA', $dstruct);

if ( $output_nvram_only )
{
	open temp_filefd, "> $outputfile";
}
else
{
	open temp_filefd, "> n.bin";
}

print temp_filefd $nvram_binary;
close(temp_filefd);

# Early exit if all we want is to dump the custom NVRAM binary
if ( $output_nvram_only )
{
	exit 0;
}
elsif ($replace_nvram)
{
	@offsets=split(/,/, $offsets_str);
	#open input file
	open ifile_fd, "< $inputfile" or die "Can't open input file " . $inputfile ;
	binmode ifile_fd;
	local $/;
	my $ifile_buffer= do { local $/; <ifile_fd>};
	close(ifile_fd);

	#if tagged add tag else just replace
	if ($nvram_magic)
	{
		$nvram_binary="nVrAmDaT".$nvram_binary;
	}
	
	my $ii=@offsets;
	while($ii > 0)
	{

		substr($ifile_buffer, $offsets[$ii-1], length($nvram_binary))=$nvram_binary; 
		$ii=$ii-1;
	}
	
	#write buffer to outputfile
	open ofile_fd, "> $outputfile"or die "Can't open output file " . $outputfile ;
	binmode ofile_fd;
	print ofile_fd $ifile_buffer;
	close(ofile_fd);
	
}
elsif ( $wholeflashfile eq "" )
{
	#we reached to a point where we read the input file tag
	#genearte outputbuffer
	#

	open ifile_fd, "< $inputfile" or die "Failed to open inputfile $inputfile\n";
	binmode ifile_fd;

	my $file_tag_len=$c->sizeof('FILE_TAG');

	# host is always little endian and we want to parse that accordingly
	my $c_ptag = Convert::Binary::C->new(%main::config)->parse_file($nvramfile);
	$c_ptag->tag('FILE_TAG.kernelAddress', Format=> 'String');
	$c_ptag->tag('FILE_TAG.kernelLen', Format=> 'String');
	$c_ptag->tag('FILE_TAG.boardId', Format=> 'String');
	$c_ptag->tag('FILE_TAG.totalImageLen', Format=> 'String');
	$c_ptag->tag('FILE_TAG.imageVersion', Format=> 'String');
	$c_ptag->tag('FILE_TAG.cfeAddress', Format=> 'String');
	$c_ptag->tag('FILE_TAG.cfeLen', Format=> 'String');
	$c_ptag->tag('FILE_TAG.rootfsAddress', Format=> 'String');
	$c_ptag->tag('FILE_TAG.rootfsLen', Format=> 'String');
	$c_ptag->tag('FILE_TAG.dtbAddress', Format=> 'String');
	$c_ptag->tag('FILE_TAG.dtbLen', Format=> 'String');
	$c_ptag->tag('FILE_TAG.tagValidationToken', Format=> 'Binary');

	my $ptag;
	my $ptag_struct;

	if ( read (ifile_fd, $ptag, $file_tag_len) == $file_tag_len)
	{
		$ptag_struct=$c_ptag->unpack('FILE_TAG', $ptag);
	}

	my $ptag_1 = substr($ptag, 0, ($file_tag_len - $token_len));
	my $Ctx = new Digest::CRC(width => 32, poly => 0x04c11db7, init => 0xffffffff, xorout => 0, refin => 1, refout => 1);
	$Ctx->add($ptag_1);
	my $tvt=0;
	$tvt=$ptag_struct->{'tagValidationToken'} or die "Illegal image ! Tag crc failed.(tagValidationToken)\n";
	my $calc_csum=$Ctx->digest;
	my $regular_csum=0xfefe;
	$regular_csum=unpack("I", $tvt);
	my $reverse_csum=0xefef;
	$reverse_csum=unpack("N", $tvt);

	if ($calc_csum != $regular_csum)
	{
		if($calc_csum != $reverse_csum)
		{
			die("Illegal image ! Tag crc failed.\n");
		}
	}

	my $kaa=$ptag_struct->{'kernelAddress'} or die "Illegal image ! Tag crc failed.(kernelAddress)\n";
	my $ulKernelOffset=$kaa-$image_base;
	$kaa=$ptag_struct->{'kernelLen'} or die "Illegal image ! Tag crc failed.(kernelLen)\n";
	my $ulKernelLen=$kaa+0;
	$kaa=$ptag_struct->{'cfeAddress'} or $kaa=$image_base;
	my $ulCfeOffset=$kaa-$image_base;
	$kaa=$ptag_struct->{'cfeLen'} or $kaa=0;
	my $ulCfeLen=$kaa+0;
	$kaa=$ptag_struct->{'rootfsAddress'} or die "Illegal image ! Tag crc failed.(rootfsAddress)\n";
	my $ulFsOffset=$kaa-$image_base;
	$kaa=$ptag_struct->{'rootfsLen'} or die "Illegal image ! Tag crc failed.(rootfsLen)\n";
	my $ulFsLen=$kaa+0;
	$kaa=$ptag_struct->{'dtbAddress'} or die "Illegal image ! Tag crc failed.(dtbAddress)\n";
	my $ulDtbOffset=$kaa-$image_base;
	$kaa=$ptag_struct->{'dtbLen'} or die "Illegal image ! Tag crc failed.(dtbLen)\n";
	my $ulDtbLen=$kaa+0;


	my $ulTagOffset = $ulFsOffset - $file_tag_len;
	my $ulTagLen = $file_tag_len;


	   printf("\tImage components offsets\n");
	    printf("\timage base              : 0x%8.8x\n",
		$image_base);
	    printf("\tCFE offset              : 0x%8.8x    -- Length: 0x%x\n",
		$ulCfeOffset, $ulCfeLen);
	    printf("\tNVRAM offset            : 0x%8.8x    -- Length: 0x%x\n",
		$ulNvramOffset + $ulCfeOffset, $ulNvramLen);
	    printf("\tfile tag offset         : 0x%8.8x    -- Length: 0x%x\n",
		$ulTagOffset, $ulTagLen);
	    printf("\trootfs offset           : 0x%8.8x    -- Length: 0x%x\n",
		$ulFsOffset, $ulFsLen);
	    printf("\tkernel offset           : 0x%8.8x    -- Length: 0x%x\n",
		$ulKernelOffset, $ulKernelLen);
	    printf("\tDTB offset              : 0x%8.8x    -- Length: 0x%x\n",
		$ulDtbOffset, $ulDtbLen);

	my $outbuf="\377" x ($ulDtbOffset + $ulDtbLen + $psi_size);

	#print ($ulDtbOffset + $ulDtbLen + $psi_size);
	#print "\n---\n";

	open outfile_fd, "> $outputfile" or die "Failed to open $outputfile";
	binmode outfile_fd;

	my $cfebin;
	if ( read (ifile_fd, $cfebin, $ulCfeLen) == $ulCfeLen)
	{
		substr($outbuf, $ulCfeOffset, $ulCfeLen, $cfebin);
	}
	else
	{
		die("File read error $inputfile");
	}

	substr($outbuf, ($ulFsOffset-$file_tag_len), length($ptag), $ptag);


	my $fs;
	if ( read (ifile_fd, $fs, $ulFsLen) == $ulFsLen)
	{
		substr($outbuf, $ulFsOffset, $ulFsLen, $fs);
	}
	else
	{
		die("File read error $inputfile");
	}

	my $kernel;
	if ( read (ifile_fd, $kernel, $ulKernelLen ) == $ulKernelLen)
	{
		substr($outbuf, $ulKernelOffset, $ulKernelLen, $kernel);
	}
	else
	{
		die("File read error $inputfile");
	}

	my $dtb;
	if ( read (ifile_fd, $dtb, $ulDtbLen ) == $ulDtbLen)
	{
		substr($outbuf, $ulDtbOffset, $ulDtbLen, $dtb);
	}
	else
	{
		die("File read error $inputfile");
	}

	substr($outbuf, $ulNvramOffset, length($nvram_binary), $nvram_binary);

	print outfile_fd $outbuf;

	my $of_size=length($outbuf);
	printf( "\tThe size of the entire flash image is %d bytes.\n", $of_size);

	close(outfile_fd);
	close(ifile_fd);

}
else
{
	open wholefile_fd, "+< $wholeflashfile" or die "Failed to open wholeflashfile $wholeflashfile\n";
	binmode wholefile_fd;
        my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
           $atime,$mtime,$ctime,$blksize,$blocks)
               = stat(wholefile_fd);
	#read everything except the wfitag
	read(wholefile_fd, $data, $size-$token_len);
	$ctx = new Digest::CRC(width => 32, poly => 0x04c11db7, init => 0xffffffff, xorout => 0, refin => 1, refout => 1);
	$ctx->add($data);
	$csum=$ctx->digest;
	#printf("Original CRC %x\n", $csum);

	my $wfi_tag;
	my $wfi_tag_struct;
	read(wholefile_fd, $wfi_tag, $token_len);
	my $c = Convert::Binary::C->new(%main::config)->parse_file($nvramfile);
	$wfi_tag_struct=$c->unpack('WFI_TAG', $wfi_tag);

	if($csum == $wfi_tag_struct->{'wfiCrc'})
	{
		substr($data, $nvram_base+$ulNvramOffset, length($nvram_binary), $nvram_binary);
		my $data_temp=$data;

		my $nvramdata_sign="nVrAmDaTaSiGnAtUrE";
		my $total_nvram_found=0;

		while($data_temp =~ m/$nvramdata_sign/g)
		{
			
			substr($data, pos($data_temp)-10, length($nvram_binary), $nvram_binary);
			$total_nvram_found=$total_nvram_found+1;
		} 
		$ctx = new Digest::CRC(width => 32, poly => 0x04c11db7, init => 0xffffffff, xorout => 0, refin => 1, refout => 1);
		$ctx->add($data);
		$csum=$ctx->digest;
		$wfi_tag_struct->{'wfiCrc'}=$csum;
		$wfi_tag=$c->pack('WFI_TAG', $wfi_tag_struct);

		seek(wholefile_fd, 0, 0);
		print wholefile_fd $data;
		print wholefile_fd $wfi_tag;
		close(wholefile_fd);
	}
	else
	{
		printf("CRC mismatch File %X - calculated %X\n", $csum, $wfi_tag_struct->{'wfiCrc'});
	}
	
}

