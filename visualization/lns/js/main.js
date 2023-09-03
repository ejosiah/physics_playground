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
            names.push(`${name} (ms)`);
        }
    }

    const metadata = [];

    for(const size of sizes){
        metadata.push({size, names, colors});
    }

    return metadata;
}

const metadata = extractSizes(benchmarks);

console.log(metadata);

const data = [];

for(let { size, names, colors }  of metadata){
    colors.push(randomColor());
    data.push({
        labels: names,
        datasets: [
            {
                label: `Linear system solvers ${size}`,
                data: [],
                borderWidth: 1,
                backgroundColor: colors
            }
        ]
    });
}

for(const entry of data){
    console.log(entry);
}


for (const benchmark of benchmarks) {
    const [_, name, size] = benchmark.name.split("/");
    const index = metadata.findIndex( e => e.size == size );
    data[index].datasets[0].data.push(benchmark.lns_iterations);

   /* data[0].labels.push(label);
    data[0].datasets[0].data.push(Math.log(benchmark.cpu_time));
    data[0].datasets[0].backgroundColor.push(randomColor());*/
}

for(const entry of data) {
    const div = document.createElement("div");
    div.innerHTML = "<canvas width='500' height='300'></canvas>"
    document.body.appendChild(div);

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