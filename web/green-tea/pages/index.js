import Head from "next/head";
import Session from "./session";
import Dashboard from "../components/Dashboard";
import Sidebar from "../components/Sidebar";

export default function Home() {
    let page = (
        <div className="flex">
            <Head>
                <title>GreenTea Dashboard</title>
                <link rel="icon" href="/greentea.ico" />
            </Head>
            <Sidebar />
            <Dashboard />
        </div>
    );

    return (<Session page={page}/>);
}
