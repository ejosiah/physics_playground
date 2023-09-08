const randomColor = () => {
    const r = Math.random() * 255;
    const g = Math.random() * 255;
    const b = Math.random() * 255;

    return `rgba(${r}, ${g}, ${b}, 0.2)`;
}

const deepCopy = source => {
    return JSON.parse(JSON.stringify(source))
}

const createControl = (metadata, runs, options = {}) => {
    const container = document.querySelector(".controls main");
    const extraControls = options.extraControls || [];
    const rename = options.rename || (it => it);
    const update = options.update || (() => {})

    let history = "";
    for(let i = 0; i < runs.length; i++) {
        history += `<span><input name="history" type="checkbox" value="${i}" checked \><label>${i}</label></span>`
    }


    let dataSize = "";
    for(let i = 0; i < metadata.length; i++) {
        const size = metadata[i].size;
        dataSize += `<span><input name="size" type="checkbox" value="${i}" checked \><label>${size}</label></span>`
    }

    let names = []
    for(const benchmark of runs[0].benchmarks){
        let [$1, name, $2] = benchmark.name.split("/");

        if(!names.find(it => it === name)){
            names.push(name)
        }
    }

    names = rename(names);

    let solvers = "";
    for(let i = 0; i < names.length; i++){
        const name = names[i];
        solvers += `<div><input name="solver" type="checkbox" value="${i}" checked \><label>${name}</label></div>`
    }

    const components = [{ history }, { dataSize }, {solvers}];

    if(extraControls.length > 0){
        for(const {position, component} of extraControls){
            components.splice(position, 0, component);
        }
    }

    const content =
        components.map( entry => {
            const [[name, content]] = Object.entries(entry);
            return `<fieldset><legend>${name}</legend>${content}</fieldset>`
        }).join("\n");

    container.innerHTML  = `<form>${content}</form>`


    container.querySelector('form').addEventListener('change', e => {
        const controls = Array.from(container.querySelectorAll(`input[type=checkbox]`));

        const staging = new Map();
        for(const control of controls){
            if(!staging.has(control.name)){
                staging.set(control.name, []);
            }
            staging.get(control.name).push(!control.checked)
            if(control.name === 'solver'){
                staging.get(control.name).push(!control.checked)
            }
        }
        const filter = {};
        for(const [key, value] of staging){
            filter[key] = value;
        }
        updateGraph(filter, update)
    });

}

const createChart = data => {
    document.querySelector('.graph').innerHTML = '';
    for(const entry of data) {
        const div = document.createElement("div");
        div.innerHTML = "<canvas width='500' height='300'></canvas>"
        document.querySelector('.graph').appendChild(div);

        new Chart(div.querySelector('canvas'), {
            type: 'bar',
            data: entry,
            options: {
                scales: {
                    y: {
                        beginAtZero: true
                    }
                }
            }
        });
    }
}
const updateGraph = (filter, update) => {
    if(!window.graph_data) return;

    const data = deepCopy(window.graph_data).filter((_, index) => !filter.size[index] );

    for(const entry of data){
        entry.datasets = entry.datasets.filter((_, index) => !filter.history[index] );

        entry.labels = entry.labels.filter((_, index) => !filter.solver[index] );
        for(const dataset of entry.datasets){
            dataset.data = dataset.data.filter((_, index) => !filter.solver[index]);
        }

        update(entry, filter);

    }
    createChart(data);
}


const constructGraph =  (metadata, history, options) => {
    const data = [];
    const hcolors =  [];


    for(let i = 0; i < history.length; i++){
        const colors = [];
        const numBenchmarks = metadata[0].names.length;
        for (let j = 0; j < numBenchmarks; j++) {
            colors.push(randomColor());
        }
        hcolors.push(colors);
    }

    for(let { size, names, colors }  of metadata){
        data.push({
            labels: names,
            datasets: []
        });
        for(let i = 0; i < history.length; i++){
            data[data.length - 1].datasets.push(            {
                label: `${options.name} ${size} run(${i})`,
                data: [],
                borderWidth: 1,
                backgroundColor: hcolors[i]
            })
        }
    }

    for(let i = 0; i < history.length; i++) {
        for (const benchmark of history[i].benchmarks) {
            const [_, name, size] = benchmark.name.split("/");
            const index = metadata.findIndex(e => e.size == size);
            data[index].datasets[i].data.push(benchmark.real_time);
        }
    }

    createChart(data);

    window.graph_data = data;
}


const extractMetaData = benchmarks => {
    const set = new Set();

    const colors = [];
    for (const benchmark of benchmarks) {
        const [_, _1, size] = benchmark.name.split("/");
        set.add(size);

    }

    const sizes = [];
    for(const size of set){
        sizes.push(size);
        colors.push(randomColor());
    }

    const names = []
    for(const benchmark of benchmarks){
        const [_, name, size] = benchmark.name.split("/");
        if(size == sizes[0]){
            names.push(`${name} (${benchmark.time_unit})`);
        }
    }

    const metadata = [];

    for(const size of sizes){
        metadata.push({ size : Number(size), names, colors});
    }

    return metadata;
}

const createGraph = async (data_path, options) => {
    const response = await fetch(data_path);
    const history = await response.json();

    const metadata = extractMetaData(history[0].benchmarks);

    createControl(metadata, history, options);
    constructGraph(metadata, history, options);
}

export default createGraph;