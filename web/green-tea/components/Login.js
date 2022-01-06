import React from "react";
import Head from "next/head";
import CONFIG from "../config.json";

class Login extends React.Component {
    constructor(props) {
        super(props);
    }

    async doRegister() {
        alert(this.email.current.value);
    }

    handleInputChange(e) {
        const target = e.target;
        const value = target.type === 'checkbox' ? target.checked : target.value;
        const name = target.name;
        this.setState({
            [name]: value
        });
    }

    async doLogin(e) {
        e.preventDefault();
        let ch = new XMLHttpRequest;
        let data = new FormData(e.target);
        ch.onload = function () {
            let json = JSON.parse(this.responseText);
            if (json.msg.is_ok) {
                alert(json.msg.msg);
                localStorage.setItem("token", json.msg.token);
                window.location = "/";
            } else {
                alert(json.msg.msg);
            }
        };
        ch.open("POST", CONFIG.BASE_API_URL + "/api.php?action=login");
        ch.send(data);
    }

    render() {
        return (
            <div className="w-screen bg-cream h-screen pt-8">
                <Head>
                    <title>GreenTea Dashboard</title>
                    <link rel="icon" href="/greentea.ico" />
                </Head>

                <main className="flex flex-col w-10/12 md:w-7/12 lg:w-6/12 xl:w-5/12 2xl:w-4/12 bg-white mx-auto px-12 py-12  space-y-5">
                    <img
                        className="w-[300px] mx-auto"
                        src="greentea.svg"
                        alt="logo"
                    />
                    <h1 className="text-gray-600 font-sans font-semibold text-[30px] text-center">
                        Login to your account
                    </h1>
                    <form onSubmit={(e) => this.doLogin(e)} className="flex flex-col space-y-5" action="" method="">
                        <div className="flex flex-col">
                            <label
                                className="text-gray-500 text-[20px] font-sans"
                                htmlFor="email"
                            >
                                Email
                            </label>
                            <input
                                className="outline-none border-2 rounded-md px-3 py-3 mt-2 focus:border-dark-green"
                                type="text"
                                id="email"
                                name="email"
                                onChange={(e) => this.handleInputChange(e)}
                                placeholder="Your email"
                                required
                            />
                        </div>
                        <div className="flex flex-col">
                            <label
                                className="text-gray-500 text-[20px] font-sans"
                                htmlFor="password"
                            >
                                Password
                            </label>
                            <input
                                className="outline-none border-2 rounded-md px-3 py-3 mt-2 focus:border-dark-green"
                                type="password"
                                id="password"
                                name="pass"
                                onChange={(e) => this.handleInputChange(e)}
                                placeholder="Your password"
                                required
                            />
                        </div>
                        <button type="submit"
                            className="bg-dark-green text-white text-[20px] px-3 py-4 rounded-lg text-center cursor-pointer hover:bg-dark-green-1"
                            href="/"
                        >
                            Login
                        </button>
                        <a href="/register">Register</a>
                    </form>
                </main>
            </div>
        );
    }
}

export default Login;
