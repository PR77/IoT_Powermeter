var dataArray = [];

loadCSV(); // Download the CSV data, load Google Charts, parse the data, and draw the chart

function loadCSV() {
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            var lines = this.responseText.split("\n");

            for (var i = 0; i < lines.length; i++) {
                var data = lines[i].split(",", 2);
                data[0] = new Date(parseInt(data[0]) * 1000);
                data[1] = parseFloat(data[1]);
                dataArray.push(data);    
            }

            document.getElementById("elements").innerText = dataArray.length;
            google.charts.load('current', { 'packages': ['line', 'corechart'] });
            google.charts.setOnLoadCallback(drawChart);
        }
    };
    xmlhttp.open("GET", "log.csv", true);
    xmlhttp.send();
}

function drawChart() {
    var data = new google.visualization.DataTable();
    data.addColumn('datetime', 'unix');
    data.addColumn('number', 'Watts');

    data.addRows(dataArray);

    var options = {
        curveType: 'function',

        height: 400,

        legend: { 
            position: 'bottom' 
        },

        vAxis: {
            title: "Power (Watts)"
        }
    };

    var chart = new google.visualization.LineChart(document.getElementById('chart_div'));

    chart.draw(data, options);

    var loadingdiv = document.getElementById("loading");
    loadingdiv.style.visibility = "hidden";
}
