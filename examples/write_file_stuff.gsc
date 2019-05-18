bubblesort(a) {
	for(j = 0; j < a.size; j+=1) {
		for(i = 1; i< a.size - j; i+=1) {
			if(a[i - 1] > a[i]) {
				tmp = a[i];
				a[i]=a[i - 1];
				a[i - 1]=tmp;
			}
		}
		println(j);
	}
}

some_print_stuff() {
	//printf is really slow unless you change some buffering options
	limit=3800;
	fn="test.bin";
	fp = fopen(fn,"w");
	for(i = 0; i < limit; i++) {
		fwritevalue(fp, string(i) + " ", false);
	}
	
	fclose(fp);
	println("Done.");
	
	str = read_text_file(fn);
	split=strtok(str," ");
	
	for(i=0;i<split.size;i++)
		println(split[i]);
	
}

write_array_to_file(fn, a) {
	fp = fopen(fn,"w"); //fp is a FILE object which automatically does fclose once fp goes out of bounds
	for(i = 0; i < a.size; i++)
		fwritevalue(fp, string(a[i]) + " ", false);
}

main() {
	a=[];
	for(i=0;i<200;i++) {
		a[i]=randomint(255);
	}
	println("Made random numbers!");
	bubblesort(a);
	println("Done.");
	
	write_array_to_file("test.bin",a);
	getchar();
}