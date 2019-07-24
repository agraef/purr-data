while(<*>){
    chomp;
    if(/darwin$/ | /liblyonpotpourri.dylib/){
	`mv $_ darwin_bin64`;
    }
}
