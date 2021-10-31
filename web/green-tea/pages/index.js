import Dashboard from "../components/Dashboard";
import Sidebar from "../components/Sidebar";

export default function Home() {
    return (
        <div className="flex">
            <Sidebar />
            <Dashboard />
        </div>
    );
}
