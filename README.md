## Archive Implementation

archive operates on the archive-file listed adding, deleting, listing contents according to the operation selected, using the files specified as appropriate. archive maintains compatibility with "ar" unility on Linux, with the exception of the "A" option which is an added option to archive. This project is not meant for creating a compression program such as zip. An archive is a single file holding a collection of other files in a structure that makes it possible to retrieve the original individual files. This is useful when creating a library. Emory Honor Code applies. 

#### Synopsis (Usage):

```bash
foo@rxluke:~$ archive [qxotA:] archive_file your_file
```

#### Description:

* -q: append the file specified to the archive file.
* -x: extract the specified files from the archive file.
* -o: used in combinition with "-x" option to restore the original permission and mtime.
* -t: list the names in the archive file.
* -A: append all regular files in the directory modified more than N days ago. N is specified by the user.

#### Example:

```bash
foo@rxluke:~$ echo one>one
foo@rxluke:~$ echo two>two
```

Above commands will create two files called one and two.

```bash
foo@rxluke:~$ archive arch one
foo@rxluke:~$ archive arch two
```

Above commands will create an archive file named arch, and append file one and file two in it.

```bash
foo@rxluke:~$ archive -x -o arch one
```

Above command will extract file one from arch, and restore its meta information such as permission and mtime.