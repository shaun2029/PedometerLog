PedometerLog
============
Copyright Mike Davies 2013

To provide Linux support for the Crivit Z31192 pedometer.

This is a GUI based version of the program.

Essentially all the program does is download data from a Crivit Z31192 USB Pedometer, and display the data as a variety of graphs.

Data can also be imported from .csv files. A sample .csv file is included.

There are things you need to know.

1) I accept no responsibility for loss of data or subsequent failure of your pedometer. I have written this program with no knowledge of the USB interface. The program appears to work OK, but I may have missed something. It is possible that this program could cause your pedometer to fail. I simply don't know enough about the format of data being sent to the pedometer and what each individual bit means - there is no documentation.

2) The program downloads the step limit, weight and step length from the pedometer. There is only one copy of these in the pedometer. If you change these values, you should be made aware that that values that appear in the database may not be right for the day in question.

3) The program initially starts up in metric mode. When data is downloaded from your pedometer, and it discovers your pedometer is operating in imperial measurements, the program will switch to imperial mode. Data is ALWAYS saved in the database in metric. It is converted during save & retrieval to imperial if needed.

Prerequisites. libusb-1 
	       mysql & udev support

There's some things you might like to know that are not obvious.

1) If you've got a mouse wheel you can zoom the graphs.
2) You can left click and drag the graphs.
3) Left clicking on a bar gives more details about the plot.
4) In the "Daily" display, right clicking on a bar allows notes to be added/displayed for that particular date.
5) Right clicking on a column in Daily view allows you to add notes for a particular day. If a note already exists for the date, then it is displayed and can be changed or deleted.
6) If Data Mining is enabled in Tools -> Configuration, then double clicking (with left mouse key) on a histogram column selects all data contained in that column, and expand it out into the next logical unit of time. e.g. In year view double click on 2012 and it expands into Jan, Feb, Mar ... etc. Double click on October expands to Week 40 thru Week 44, double click on Week 42 and it will expand to 15th Oct to 21st Oct. Double click an a day and it goes back to showing all data in year view. If you want to see all of September 2012, double click on 2012, then September, and while it shows the week view, select a "Daily" view from the drop down menu.

KNOWN PROBLEMS

1) There is no validation of data during the import of a CSV.
2) If you import a CSV in imperial units, it will corrupt the database. The database is always assumed to be metric. Currently the program does not know if the CSV being imported is metric or imperial.
3) Tools->Recalculate is a bit iffy. You will probably want to export a copy of your database to a .csv file before you try this out.

Proposed updates for the later versions.

1) Output to .pdf, .png etc
2) Multi-user support
3) ???
