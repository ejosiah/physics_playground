import {randomColor, sequence} from "./util.js"

const generateChartConfig = (field, source, scale = 1, offset = 0) => {
    const configs = []
    const labels = sequence(100 - offset, 1 + offset);
    const colors = ['rgb(255, 99, 132)', 'rgb(255, 205, 86)', 'rgb(54, 162, 235)', 'rgb(75, 192, 192)']
    for(const [dataSize, entries] of Object.entries(source)) {
        const config = {
            type : 'line',
            data: {
                labels,
                datasets: []
            },
            options : {
                plugins : {
                    title: {display : true, text: field}
                }
            }
        };
        let i = 0;
        for(const entry of entries){
            config.data.datasets.push({
                label : `${entry.solver} (${dataSize})`,
                data : entry[field].map(it => it * scale).slice(offset),
                fill : false,
                borderColor : colors[i++],
                tension: 0.1
            })
        }
        configs.push(config);
    }
    return configs;
}

const createChart = configs => {
    document.querySelector('.graph').innerHTML = '';
    for(const config of configs) {
        const div = document.createElement("div");
        div.innerHTML = "<canvas width='500' height='300'></canvas>"
        document.querySelector('.graph').appendChild(div);

        new Chart(div.querySelector('canvas'), config);
    }
}

const resp = await fetch("./data/linear_solver_convergence.json");
const data = await resp.json();

const loadGraph = data => {
    const form = document.querySelector('form');
    const field = form.querySelector('input[name=field]:checked')
    const scale = field.dataset.scale || 1;

    if(window.previous_field !== field.value) {
        const configs = generateChartConfig(field.value, data, scale);
        createChart(configs);
        window.previous_field = field.value
    }
}

loadGraph(data);
document.querySelector('form').addEventListener('click', e => {
    loadGraph(data);
});