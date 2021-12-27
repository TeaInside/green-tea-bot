module.exports = {
    mode: "jit",
    purge: ["./pages/**/*.{js,ts,jsx,tsx}", "./components/**/*.{js,ts,jsx,tsx}"],
    darkMode: false, // or 'media' or 'class'
    theme: {
        extend: {
            fontFamily: {
                sans: ["Source Sans Pro"],
            },
            colors: {
                "dark-green": "#52734D",
                "dark-green-1": "#7fa979",
                cream: "#F3EFCC",
            },
        },
    },
    variants: {
        extend: {},
    },
    plugins: [require("tailwind-scrollbar")],
};
