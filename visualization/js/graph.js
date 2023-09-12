import {randomColor, deepCopy} from "./util.js"
const extractPredicates = container => {
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
    const predicates = {};
    for(const [key, value] of staging){
        predicates[key] = value;
    }
    return predicates;
}

const createControl = (metadata, runs, options = {}) => {
    const container = document.querySelector(".controls main");
    const extraControls = options.extraControls || [];
    const rename = options.rename || (it => it);
    const update = options.update || (() => {})
    const extraFields = options.fields || [];

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

    let fields =
        `<div>
            <input type="radio" name="field" value="real_time"><label>real time</label>
            <input type="radio" name="field" value="cpu_time" checked="checked"><label>cpu time</label>
            ${extraFields.map( ({ display, name }) => `<input type="radio" name="field" value="${name}" ><label>${display}</label>`)}
        </div>`

    const components = [{ history }, { dataSize }, {solvers}, {fields} ];

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
        const selectedField = container.querySelector('input[type=radio]:checked')?.value;
        if(selectedField && selectedField !== window.previous_field){
            const { metadata, history, options } = window.createGraphArgs;
            constructGraph(metadata, history, options, selectedField);
        }

        const predicate = extractPredicates(container);
        updateGraph(predicate, update)
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

const filter = (data, predicates, interceptor) => {
    data = data.filter((_, index) => !predicates.size[index] );
    for(const entry of data) {
        entry.datasets = entry.datasets.filter((_, index) => !predicates.history[index] );

        entry.labels = entry.labels.filter((_, index) => !predicates.solver[index] );
        for(const dataset of entry.datasets){
            dataset.data = dataset.data.filter((_, index) => !predicates.solver[index]);
        }

        interceptor(entry, predicates);
    }
    return data;
}
const updateGraph = (predicates, interceptor) => {
    if(!window.graph_data) return;

    const data = filter(deepCopy(window.graph_data), predicates, interceptor);
    createChart(data);
}


const constructGraph =  (metadata, history, options, field = "cpu_time") => {
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
            data[index].datasets[i].data.push(benchmark[field]);
        }
    }

    createChart(data);

    window.graph_data = data;
    window.previous_field = field;
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
    window.createGraphArgs = { metadata, history, options };

    createControl(metadata, history, options);
    constructGraph(metadata, history, options);
}

export default createGraph;