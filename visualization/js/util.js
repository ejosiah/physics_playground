const randomColor = (alpha = 0.2) => {
    const r = Math.random() * 255;
    const g = Math.random() * 255;
    const b = Math.random() * 255;

    return `rgba(${r}, ${g}, ${b}, ${alpha})`;
}

const deepCopy = source => {
    return JSON.parse(JSON.stringify(source))
}

const sequence = (size, start = 0) => {
    const data = [];
    for(let i = start; i < (start + size); i++){
        data.push(i);
    }
    return data;
}


export {randomColor, deepCopy, sequence}