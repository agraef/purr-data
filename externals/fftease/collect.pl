while(<*>){
    chomp;
    if(/darwin$/ || /libfftease.dylib/){
	`mv $_ fftease32-externals`;
    }
}
