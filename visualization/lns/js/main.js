const randomColor = () => {
    const r = Math.random() * 255;
    const g = Math.random() * 255;
    const b = Math.random() * 255;

    return `rgba(${r}, ${g}, ${b}, 0.2)`;
}

const response = await fetch("./data/benchmarks.json");
const history = await response.json();
const benchmarks = history[0].benchmarks;


const extractSizes = benchmarks => {
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

const metadata = extractSizes(benchmarks);

console.log(metadata);

const data = [];
const hcolors = [];
// const alldatasets = [];
for(let i = 0; i < history.length; i++){
    const colors = [];
    for(let j = 0; j < metadata.length; j++){
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
            label: `Linear system solvers ${size} run(${i})`,
            data: [],
            borderWidth: 1,
            backgroundColor: hcolors[i]
        })
    }
}

for(const entry of data){
    console.log(entry);
}

for(let i = 0; i < history.length; i++) {
    for (const benchmark of history[i].benchmarks) {
        const [_, name, size] = benchmark.name.split("/");
        const index = metadata.findIndex(e => e.size == size);
        data[index].datasets[i].data.push(benchmark.real_time);


    }
}

for(const entry of data) {
    const div = document.createElement("div");
    div.innerHTML = "<canvas width='500' height='300'></canvas>"
    document.querySelector('main').appendChild(div);

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